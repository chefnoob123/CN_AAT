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

// Function to handle errors
void error(const char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) {
  const char *key = "ipsum"; // The encryption key

  if (argc < 2) {
    fprintf(stderr, "Please provide the port number. exiting....\n");
    exit(1);
  }
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("Could not Open a Socket");
  }

  char buffer[256]; // A buffer to read and write messages
  struct sockaddr_in serv_addr, client_addr;
  socklen_t client_len;

  bzero((char *)&serv_addr, sizeof(serv_addr));
  int port_no = atoi(argv[1]);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port_no);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    error("Binding failed");
  }

  listen(sockfd, 5);
  printf("Server listening on port %d...\n", port_no);
  client_len = sizeof(client_addr);

  int new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
  if (new_sockfd < 0) {
    error("Error on Accept");
  }
  printf("Client successfully connected\n");

  // Logic for Communication
  while (1) {
    bzero(buffer, 256);

    // --- READ FROM CLIENT ---
    int n = read(new_sockfd, buffer, 255); // Read encrypted message
    if (n < 0) {
      error("Error on read");
    }
    if (n == 0) {
      printf("Client disconnected.\n");
      break;
    }
    buffer[n] = '\0'; // Null-terminate the received data

    printf("Client (Encrypted): %s\n", buffer);
    xor_cipher(buffer, n, key); // Decrypt the message
    printf("Client (Real):      %s\n", buffer);

    // Check if client said bye
    if (strcasecmp(buffer, "bye") == 0) {
      printf("Client said bye. Closing connection.\n");
      break;
    }

    // --- WRITE TO CLIENT ---
    bzero(buffer, 256);
    printf("Server (Real):      ");
    fgets(buffer, 255, stdin);

    // Remove newline
    buffer[strcspn(buffer, "\n")] = 0;
    size_t msg_len = strlen(buffer); // Get length *before* encrypting

    // Check if server said bye (check *before* encrypting)
    int i = strcasecmp(buffer, "bye");

    // Encrypt the message
    xor_cipher(buffer, msg_len, key);
    printf("Server (Encrypted): %s\n", buffer);

    // Write the encrypted message
    // We use msg_len because strlen might fail if XOR creates a null byte
    n = write(new_sockfd, buffer, msg_len);
    if (n < 0) {
      error("Error on write");
    }

    // Now break if we typed "bye"
    if (i == 0) {
      printf("Server said bye. Closing.\n");
      break;
    }
  }

  close(new_sockfd);
  close(sockfd);
  return 0;
}
