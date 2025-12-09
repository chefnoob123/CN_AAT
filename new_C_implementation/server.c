#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// --- Encryption/Decryption Implementation ---
void encrypt_data(const char *input, char *output) {
  int i = 0;
  while (input[i] != '\0') {
    output[i] = input[i] + 3;
    i++;
  }
  output[i] = '\0';
}

void decrypt_data(const char *input, char *output) {
  int i = 0;
  while (input[i] != '\0') {
    output[i] = input[i] - 3;
    i++;
  }
  output[i] = '\0';
}
// --- End Encryption ---

// --- Checksum Implementation ---
uint32_t calculate_checksum(const void *buf, size_t len) {
  const uint8_t *p = (const uint8_t *)buf;
  uint32_t checksum = 0;
  while (len--) {
    checksum += *p++;
  }
  return checksum;
}
// --- End Checksum ---

void error(const char *msg) {
  perror(msg);
  exit(1);
}

void string_to_binary(const char *input, char *output_bin) {
  int output_index = 0;
  for (size_t i = 0; i < strlen(input); i++) {
    char c = input[i];
    for (int j = 7; j >= 0; j--) {
      if ((c >> j) & 1)
        output_bin[output_index++] = '1';
      else
        output_bin[output_index++] = '0';
    }
  }
  output_bin[output_index] = '\0';
}

void binary_to_string(const char *input_bin, char *output_str) {
  size_t bin_len = strlen(input_bin);
  int output_index = 0;
  for (size_t i = 0; i < bin_len; i += 8) {
    char byte = 0;
    for (int j = 0; j < 8; j++) {
      byte = (byte << 1);
      if (i + j < bin_len && input_bin[i + j] == '1')
        byte |= 1;
    }
    output_str[output_index++] = byte;
  }
  output_str[output_index] = '\0';
}

void send_frame(int sockfd, const char *payload, char *frame_buf) {
  uint32_t chk = calculate_checksum(payload, strlen(payload));
  bzero(frame_buf, 2100);
  snprintf(frame_buf, 2100, "%s|%u", payload, chk);

  printf("Server (Frame):  ...%s|%u\n",
         strlen(payload) > 50 ? payload + strlen(payload) - 50 : payload, chk);

  int n = write(sockfd, frame_buf, strlen(frame_buf));
  if (n < 0)
    error("ERROR writing to socket");
}

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  socklen_t clilen;

  char buffer[256];
  char enc_buffer[256];
  char payload_buffer[2049];
  char frame_buffer[2100];

  struct sockaddr_in serv_addr, cli_addr;
  int n;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  listen(sockfd, 5);
  clilen = sizeof(cli_addr);
  printf("Server waiting (Port %d)...\n", portno);

  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (newsockfd < 0)
    error("ERROR on accept");
  printf("Client connected. Encryption Enabled.\n");

  while (1) {
    bzero(frame_buffer, 2100);
    n = read(newsockfd, frame_buffer, 2099);
    if (n <= 0)
      break;
    frame_buffer[n] = '\0';

    char *chk_separator = strrchr(frame_buffer, '|');
    if (!chk_separator)
      continue;

    *chk_separator = '\0';
    char *payload_part = frame_buffer;
    char *chk_part = chk_separator + 1;

    uint32_t received_chk = (uint32_t)strtoul(chk_part, NULL, 10);
    uint32_t calculated_chk =
        calculate_checksum(payload_part, strlen(payload_part));

    if (received_chk != calculated_chk) {
      printf("Client (Error):  Checksum mismatch! Sending NACK.\n");

      // Send Encrypted NACK
      bzero(enc_buffer, 256);
      encrypt_data("NACK", enc_buffer); // Encrypt "NACK"
      printf("Server (Encrypted NACK): %s\n", enc_buffer);

      bzero(payload_buffer, 2049);
      string_to_binary(enc_buffer, payload_buffer); // Binary Encode

      send_frame(newsockfd, payload_buffer, frame_buffer);
      continue;
    }

    // Checksum OK
    // 1. Binary to Encrypted String
    bzero(enc_buffer, 256);
    binary_to_string(payload_part, enc_buffer);
    printf("Client (Encrypted): %s\n", enc_buffer); // SHOW ENCRYPTED STRING

    // 2. Decrypt
    bzero(buffer, 256);
    decrypt_data(enc_buffer, buffer);

    printf("Client (Decrypted): %s\n", buffer);

    if (strcasecmp(buffer, "bye") == 0)
      break;

    // --- REPLY ---
    bzero(buffer, 256);
    printf("Server (Input):     ");
    fgets(buffer, 255, stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    // 1. Encrypt Reply
    bzero(enc_buffer, 256);
    encrypt_data(buffer, enc_buffer);
    printf("Server (Encrypted): %s\n", enc_buffer); // SHOW ENCRYPTED STRING

    // 2. Binary Encode
    bzero(payload_buffer, 2049);
    string_to_binary(enc_buffer, payload_buffer);

    // 3. Send Frame
    send_frame(newsockfd, payload_buffer, frame_buffer);

    if (strcasecmp(buffer, "bye") == 0)
      break;
  }
  close(newsockfd);
  close(sockfd);
  return 0;
}
