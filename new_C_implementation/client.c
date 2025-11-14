/*
 * A simple TCP client that communicates using binary-encoded strings
 * with CRC32 error checking.
 *
 * Protocol:
 * 1. Prompts user for text.
 * 2. Encodes text to binary payload, calculates CRC.
 * 3. Sends frame: [binary_payload]|[crc_checksum]
 * 4. Waits for a reply frame from the server.
 * 5. Validates the received frame's CRC.
 * 6. If the message is "NACK", it means the server received the last
 * frame incorrectly, so it jumps to resend the message.
 * 7. If CRC is valid and not "NACK", decodes and prints the server's reply.
 */
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h> // For uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // For bzero, bcopy
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// --- CRC32 Implementation ---
// Standard CRC32 polynomial
#define CRC32_POLY 0xEDB88320

/**
 * @brief Calculates the CRC32 checksum for a given buffer.
 * @param buf The data buffer.
 * @param len The length of the data in bytes.
 * @return The 32-bit CRC checksum.
 */
uint32_t crc32(const void *buf, size_t len) {
  const uint8_t *p = (const uint8_t *)buf;
  uint32_t crc = 0xFFFFFFFF; // Start with all 1s

  while (len--) {
    crc ^= *p++;
    for (int i = 0; i < 8; i++) {
      // If the least significant bit is 1, XOR with the polynomial
      crc = (crc >> 1) ^ (-(crc & 1) & CRC32_POLY);
    }
  }
  return crc ^ 0xFFFFFFFF; // Invert all bits at the end
}
// --- End CRC32 ---

// --- Helper Functions for Binary <-> String Conversion ---

/**
 * @brief Utility function to print an error message and exit.
 */
void error(const char *msg) {
  perror(msg);
  exit(1);
}

/**
 * @brief Converts a standard null-terminated string into a binary string.
 *
 * @param input The plaintext string (e.g., "Hi").
 * @param output_bin The output buffer. Must be at least (strlen(input) * 8) + 1
 * bytes. Will contain the binary string (e.g., "0100100001101001").
 */
void string_to_binary(const char *input, char *output_bin) {
  int output_index = 0;
  for (size_t i = 0; i < strlen(input); i++) {
    char c = input[i];
    // Iterate 8 times, for 8 bits (from MSB to LSB)
    for (int j = 7; j >= 0; j--) {
      // Check if the j-th bit is set
      if ((c >> j) & 1) {
        output_bin[output_index++] = '1';
      } else {
        output_bin[output_index++] = '0';
      }
    }
  }
  // Null-terminate the resulting binary string
  output_bin[output_index] = '\0';
}

/**
 * @brief Converts a binary string (e.g., "01000001") back to a plaintext
 * string.
 *
 * @param input_bin The binary string. Its length MUST be a multiple of 8.
 * @param output_str The output buffer for the plaintext string. Must be at
 * least (strlen(input_bin) / 8) + 1 bytes.
 */
void binary_to_string(const char *input_bin, char *output_str) {
  size_t bin_len = strlen(input_bin);
  int output_index = 0;

  if (bin_len % 8 != 0 && strlen(input_bin) > 0) {
    fprintf(stderr,
            "Warning: Binary string length (%zu) is not a multiple of 8.\n",
            bin_len);
  }

  // Process in 8-bit (1-byte) chunks
  for (size_t i = 0; i < bin_len; i += 8) {
    char byte = 0;
    // Build the byte from the 8 chars
    for (int j = 0; j < 8; j++) {
      byte = (byte << 1); // Shift left
      if (i + j < bin_len && input_bin[i + j] == '1') {
        byte = byte | 1; // Set the bit
      }
    }
    output_str[output_index++] = byte;
  }
  // Null-terminate the resulting plaintext string
  output_str[output_index] = '\0';
}

/**
 * @brief Sends a complete, CRC-checked frame to the socket.
 * @param sockfd The socket file descriptor.
 * @param payload The binary string payload to send.
 * @param frame_buf A temporary buffer for frame construction.
 */
void send_frame(int sockfd, const char *payload, char *frame_buf) {
  // Calculate CRC on the binary payload
  uint32_t crc = crc32(payload, strlen(payload));

  // Create the full frame: [payload]|[crc]
  bzero(frame_buf, 2100);
  snprintf(frame_buf, 2100, "%s|%u", payload, crc);

  printf("Client (Frame):     ...%s|%u\n",
         strlen(payload) > 50 ? payload + strlen(payload) - 50 : payload, crc);

  // Write the binary frame
  int n = write(sockfd, frame_buf, strlen(frame_buf));
  if (n < 0) {
    error("Error on Writing");
  }
}

// --- Main Client Function ---
int main(int argc, char *argv[]) {
  int sockfd, portno;
  struct sockaddr_in server_addr;
  struct hostent *server;

  // Buffers
  char buffer[256];          // Buffer for plaintext
  char payload_buffer[2049]; // Buffer for binary string payload
  char frame_buffer[2100];   // Buffer for full frame (payload + '|' + crc)

  if (argc < 3) {
    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    exit(1);
  }

  // 1. Create socket
  portno = atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("Error opening Socket");
  }

  // 2. Find server
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    error("No Such Host");
  }

  // 3. Set up server address and connect
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr,
        server->h_length);
  server_addr.sin_port = htons(portno);

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    error("Error Connecting");
  }
  printf("Successfully connected. Type 'bye' to exit.\n");

  // 4. Main communication loop
  while (1) {
  send_message: // Label to jump to for resending
    // --- WRITE TO SERVER ---
    bzero(buffer, 256);
    printf("Client (Real):      ");
    fgets(buffer, 255, stdin);

    // Remove newline
    buffer[strcspn(buffer, "\n")] = 0;

    // Check if client wants to quit
    int i = strcasecmp(buffer, "bye");

    // Convert plaintext to binary string
    bzero(payload_buffer, 2049);
    string_to_binary(buffer, payload_buffer);

    // Send the frame
    send_frame(sockfd, payload_buffer, frame_buffer);

    // Now break if we typed "bye"
    if (i == 0) {
      break;
    }

    // --- READ FROM SERVER ---
    bzero(frame_buffer, 2100);
    // Read the binary message
    int n = read(sockfd, frame_buffer, 2099);
    if (n < 0) {
      error("Error on Reading");
    }
    if (n == 0) {
      printf("Server disconnected.\n");
      break;
    }
    frame_buffer[n] = '\0'; // Null-terminate

    // --- VALIDATE FRAME ---
    // Find the CRC separator
    char *crc_separator = strrchr(frame_buffer, '|');
    if (crc_separator == NULL) {
      printf("Server (Error):  Invalid frame format. Discarding.\n");
      continue; // Wait for a new message
    }

    // Split the frame
    *crc_separator = '\0'; // Terminate the payload part
    char *payload_part = frame_buffer;
    char *crc_part = crc_separator + 1;

    // Verify CRC
    uint32_t received_crc = (uint32_t)strtoul(crc_part, NULL, 10);
    uint32_t calculated_crc = crc32(payload_part, strlen(payload_part));

    if (received_crc != calculated_crc) {
      printf(
          "Server (Error):  CRC mismatch! (Got: %u, Calc: %u). Discarding.\n",
          received_crc, calculated_crc);
      continue; // Go back to waiting for a valid message
    }

    printf("Server (Frame):     ...%s|%u (CRC OK)\n",
           strlen(payload_part) > 50 ? payload_part + strlen(payload_part) - 50
                                     : payload_part,
           received_crc);

    // Convert binary string back to plaintext
    bzero(buffer, 256);
    binary_to_string(payload_part, buffer);

    // --- CHECK FOR NACK ---
    if (strcasecmp(buffer, "NACK") == 0) {
      printf(
          "Server (Info):   Our last message was corrupted. Please re-send.\n");
      goto send_message; // Jump back to send the message again
    }

    // Print the server's real message
    printf("Server (Real):      %s\n", buffer);

    // Check if server said bye
    if (strcasecmp(buffer, "bye") == 0) {
      printf("Server said bye. Closing connection.\n");
      break;
    }
  }

  // 5. Close socket
  close(sockfd);
  return 0;
}
