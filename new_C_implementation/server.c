#include <netinet/in.h>
#include <stdint.h> // For uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // For bzero
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// --- Checksum Implementation ---
/**
 * @brief Calculates a simple checksum (sum of bytes) for a given buffer.
 * @param buf The data buffer.
 * @param len The length of the data in bytes.
 * @return The 32-bit checksum.
 */
uint32_t calculate_checksum(const void *buf, size_t len) {
  const uint8_t *p = (const uint8_t *)buf;
  uint32_t checksum = 0;

  while (len--) {
    checksum += *p++;
  }
  return checksum;
}
// --- End Checksum ---

/**
 * @brief Utility function to print an error message and exit.
 */
void error(const char *msg) {
  perror(msg);
  exit(1);
}

/**
 * @brief Converts a standard null-terminated string into a binary string.
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
 * @brief Converts a binary string back to a plaintext string.
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
 * @brief Sends a complete, Checksum-checked frame to the socket.
 * @param sockfd The socket file descriptor.
 * @param payload The binary string payload to send.
 * @param frame_buf A temporary buffer for frame construction.
 */
void send_frame(int sockfd, const char *payload, char *frame_buf) {
  // Calculate Checksum on the binary payload
  uint32_t chk = calculate_checksum(payload, strlen(payload));

  // Create the full frame: [payload]|[checksum]
  bzero(frame_buf, 2100);
  snprintf(frame_buf, 2100, "%s|%u", payload, chk);

  printf("Server (Frame):  ...%s|%u\n",
         strlen(payload) > 50 ? payload + strlen(payload) - 50 : payload, chk);

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
  char frame_buffer[2100];   // For full frame (payload + '|' + checksum)

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
    // Find the last '|' which separates payload from Checksum
    char *chk_separator = strrchr(frame_buffer, '|');
    if (chk_separator == NULL) {
      printf("Client (Error):  Invalid frame format. Discarding.\n");
      continue; // Wait for a new, valid message
    }

    // Split the frame into payload and Checksum parts
    *chk_separator = '\0'; // Terminate the payload part
    char *payload_part = frame_buffer;
    char *chk_part = chk_separator + 1;

    // Verify Checksum
    uint32_t received_chk = (uint32_t)strtoul(chk_part, NULL, 10);
    uint32_t calculated_chk =
        calculate_checksum(payload_part, strlen(payload_part));

    if (received_chk != calculated_chk) {
      // --- CHECKSUM MISMATCH: SEND NACK (Negative Acknowledgement) ---
      printf("Client (Error):  Checksum mismatch! (Got: %u, Calc: %u). Sending "
             "NACK.\n",
             received_chk, calculated_chk);

      bzero(payload_buffer, 2049);
      string_to_binary("NACK", payload_buffer);

      // Use the helper function to send the NACK frame
      send_frame(newsockfd, payload_buffer, frame_buffer);
      continue; // Go back to reading, skipping the rest of the loop
    }

    // --- CHECKSUM OK (Implicit ACK) ---
    // Print a truncated version of the binary payload for readability
    printf("Client (Frame):  ...%s|%u (Checksum OK)\n",
           strlen(payload_part) > 50 ? payload_part + strlen(payload_part) - 50
                                     : payload_part,
           received_chk);

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

    // Send the complete frame (payload + Checksum)
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
