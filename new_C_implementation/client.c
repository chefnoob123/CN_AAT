#include <netdb.h>
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
// Simple Caesar Cipher (Shift +3)
void encrypt_data(const char *input, char *output) {
  int i = 0;
  while (input[i] != '\0') {
    output[i] = input[i] + 3; // Shift char by 3
    i++;
  }
  output[i] = '\0';
}

void decrypt_data(const char *input, char *output) {
  int i = 0;
  while (input[i] != '\0') {
    output[i] = input[i] - 3; // Reverse shift
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

  printf("Client (Frame):      ...%s|%u\n",
         strlen(payload) > 50 ? payload + strlen(payload) - 50 : payload, chk);

  int n = write(sockfd, frame_buf, strlen(frame_buf));
  if (n < 0)
    error("Error on Writing");
}

int main(int argc, char *argv[]) {
  int sockfd, portno;
  struct sockaddr_in server_addr;
  struct hostent *server;

  char buffer[256];          // Plaintext
  char enc_buffer[256];      // Encrypted text
  char payload_buffer[2049]; // Binary
  char frame_buffer[2100];   // Frame

  if (argc < 3) {
    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    exit(1);
  }

  portno = atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("Error opening Socket");

  server = gethostbyname(argv[1]);
  if (server == NULL)
    error("No Such Host");

  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr,
        server->h_length);
  server_addr.sin_port = htons(portno);

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    error("Error Connecting");

  printf("Connected. Encryption Enabled (Caesar Cipher +3).\n");

  while (1) {
  send_message:
    bzero(buffer, 256);
    printf("Client (Input):      ");
    fgets(buffer, 255, stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    int i = strcasecmp(buffer, "bye");

    // 1. Encrypt
    bzero(enc_buffer, 256);
    encrypt_data(buffer, enc_buffer);
    printf("Client (Encrypted):  %s\n", enc_buffer); // SHOW ENCRYPTED STRING

    // 2. Convert Encrypted data to Binary
    bzero(payload_buffer, 2049);
    string_to_binary(enc_buffer, payload_buffer);

    // 3. Send Frame
    send_frame(sockfd, payload_buffer, frame_buffer);

    if (i == 0)
      break;

    // --- READ FROM SERVER ---
    bzero(frame_buffer, 2100);
    int n = read(sockfd, frame_buffer, 2099);
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
      printf("Client: Checksum Error. Discarding.\n");
      continue;
    }

    // 4. Convert Binary to Encrypted String
    bzero(enc_buffer, 256);
    binary_to_string(payload_part, enc_buffer);
    printf("Server (Encrypted):  %s\n", enc_buffer); // SHOW ENCRYPTED STRING

    // 5. Decrypt
    bzero(buffer, 256);
    decrypt_data(enc_buffer, buffer);

    // Check for NACK (NACK is sent encrypted, so we check decrypted value)
    if (strcasecmp(buffer, "NACK") == 0) {
      printf("Server requested retransmission (NACK received).\n");
      goto send_message;
    }

    printf("Server (Decrypted):  %s\n", buffer);

    if (strcasecmp(buffer, "bye") == 0)
      break;
  }
  close(sockfd);
  return 0;
}
