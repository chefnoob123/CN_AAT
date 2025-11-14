/*
 * A simple TCP server that communicates using binary-encoded strings
 * with CRC32 error checking.
 *
 * Protocol:
 * 1. Receives a frame: [binary_payload]|[crc_checksum]
 * 2. Validates the payload against the checksum.
 * 3. If invalid, sends a "NACK" frame.
 * 4. If valid, decodes the payload, prints it, and prompts for a reply.
 * 5. Encodes the reply, calculates its CRC, and sends a new frame.
 * This reply acts as the ACK for the client's last message.
 */
#include <netinet/in.h>
#include <stdint.h> // For uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // For bzero
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

  printf("Server (Frame):  ...%s|%u\n",
         strlen(payload) > 50 ? payload + strlen(payload) - 50 : payload, crc);

  // Send the binary frame
  int n = write(sockfd, frame_buf, strlen(frame_buf));
  if (n < 0)
    error("ERROR writing to socket");
}

// --- Main Server Function ---
int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  socklen_t clilen;

  // Buffers
  char buffer[256];          // For plaintext
  char payload_buffer[2049]; // For binary string payload
  char frame_buffer[2100];   // For full frame (payload + '|' + crc)

  struct sockaddr_in serv_addr, cli_addr;
  int n;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 1. Create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  // 2. Initialize and bind socket
  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  // 3. Listen for connections
  listen(sockfd, 5);
  clilen = sizeof(cli_addr);
  printf("Server waiting for connection on port %d...\n", portno);

  // 4. Accept a connection
  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (newsockfd < 0)
    error("ERROR on accept");
  printf("Client connected. Type 'bye' to exit.\n");

  // 5. Main communication loop
  while (1) {
    // --- READ FROM CLIENT ---
    bzero(frame_buffer, 2100);
    // Read the full frame from the client
    n = read(newsockfd, frame_buffer, 2099);
    if (n < 0)
      error("ERROR reading from socket");
    if (n == 0) {
      printf("Client disconnected.\n");
      break;
    }
    frame_buffer[n] = '\0'; // Ensure null termination

    // --- VALIDATE FRAME ---
    // Find the last '|' which separates payload from CRC
    char *crc_separator = strrchr(frame_buffer, '|');
    if (crc_separator == NULL) {
      printf("Client (Error):  Invalid frame format. Discarding.\n");
      continue; // Wait for a new, valid message
    }

    // Split the frame into payload and CRC parts
    *crc_separator = '\0'; // Terminate the payload part
    char *payload_part = frame_buffer;
    char *crc_part = crc_separator + 1;

    // Verify CRC
    uint32_t received_crc = (uint32_t)strtoul(crc_part, NULL, 10);
    uint32_t calculated_crc = crc32(payload_part, strlen(payload_part));

    if (received_crc != calculated_crc) {
      // --- CRC MISMATCH: SEND NACK (Negative Acknowledgement) ---
      printf(
          "Client (Error):  CRC mismatch! (Got: %u, Calc: %u). Sending NACK.\n",
          received_crc, calculated_crc);

      bzero(payload_buffer, 2049);
      string_to_binary("NACK", payload_buffer);

      // Use the helper function to send the NACK frame
      send_frame(newsockfd, payload_buffer, frame_buffer);
      continue; // Go back to reading, skipping the rest of the loop
    }

    // --- CRC OK (Implicit ACK) ---
    // Print a truncated version of the binary payload for readability
    printf("Client (Frame):  ...%s|%u (CRC OK)\n",
           strlen(payload_part) > 50 ? payload_part + strlen(payload_part) - 50
                                     : payload_part,
           received_crc);

    // Convert binary string back to plaintext
    bzero(buffer, 256);
    binary_to_string(payload_part, buffer);
    printf("Client (Real):   %s\n", buffer);

    // Check if client said bye
    if (strcasecmp(buffer, "bye") == 0) {
      printf("Client said bye. Closing connection.\n");
      break;
    }

    // --- WRITE TO CLIENT (This is the ACKNOWLEDGEMENT) ---
    bzero(buffer, 256);
    printf("Server (Real):   ");
    fgets(buffer, 255, stdin);
    buffer[strcspn(buffer, "\n")] = 0; // Remove newline

    // Convert plaintext reply to binary string
    bzero(payload_buffer, 2049);
    string_to_binary(buffer, payload_buffer);

    // Send the complete frame (payload + CRC)
    send_frame(newsockfd, payload_buffer, frame_buffer);

    // Check if server wants to quit
    if (strcasecmp(buffer, "bye") == 0) {
      printf("Server said bye. Closing connection.\n");
      break;
    }
  }

  // 6. Close sockets
  close(newsockfd);
  close(sockfd);
  return 0;
}
