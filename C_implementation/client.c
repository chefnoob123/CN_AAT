#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// --- XOR Cipher Function ---
// Note: This modifies the data in-place.
void xor_cipher(char *data, size_t len, const char *key) {
  size_t key_len = strlen(key);
  for (size_t i = 0; i < len; i++) {
    data[i] = data[i] ^ key[i % key_len];
  }
}

void error(const char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) {
  const char *key = "ipsum"; // The encryption key

  struct sockaddr_in server_addr;
  struct hostent *server;
  char buffer[256];

  if (argc < 3) {
    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    exit(1);
  }
  int portno = atoi(argv[2]);
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    error("Error opening Socket");
  }
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    error("No Such Host");
  }

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

  while (1) {
    // --- WRITE TO SERVER ---
    bzero(buffer, 256);
    printf("Client (Real):      ");
    fgets(buffer, 255, stdin);

    // Remove newline
    buffer[strcspn(buffer, "\n")] = 0;
    size_t msg_len = strlen(buffer); // Get length *before* encrypting

    // Check if client wants to quit (check *before* encrypting)
    int i = strcasecmp(buffer, "bye");

    // Encrypt the message
    xor_cipher(buffer, msg_len, key);
    printf("Client (Encrypted): %s\n", buffer);

    // Write the encrypted message
    // CRITICAL FIX: Use msg_len, NOT sizeof(buffer)
    int n = write(sockfd, buffer, msg_len);
    if (n < 0) {
      error("Error on Writing");
    }

    // Now break if we typed "bye"
    if (i == 0) {
      break;
    }

    // --- READ FROM SERVER ---
    bzero(buffer, 256);
    n = read(sockfd, buffer, 255); // Read encrypted message
    if (n < 0) {
      error("Error on Reading");
    }
    if (n == 0) {
      printf("Server disconnected.\n");
      break;
    }
    buffer[n] = '\0'; // Null-terminate

    printf("Server (Encrypted): %s\n", buffer);
    xor_cipher(buffer, n, key); // Decrypt the message
    printf("Server (Real):      %s\n", buffer);

    // Check if server said bye
    if (strcasecmp(buffer, "bye") == 0) {
      printf("Server said bye. Closing connection.\n");
      break;
    }
  }
  close(sockfd);
  return 0;
}
