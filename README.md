# Simple Encrypted C Chat (Client-Server)

This is a basic client-server chat application written in C. It establishes a TCP connection between a single client and a server, allowing for full-duplex (two-way) communication.

All messages sent between the client and server are encrypted using a simple, symmetric XOR cipher with a hardcoded key.

## Features

* **TCP Socket Communication:** Uses standard Berkeley sockets for reliable, connection-oriented chat.
* **Full-Duplex Chat:** The server uses the `select()` system call to monitor both the client socket and its own terminal (`stdin`) for input. This allows both the server operator and the client to type and send messages at any time without blocking each other.
* **XOR Encryption:** All messages are encrypted/decrypted using a simple XOR cipher.
    * **Key:** `ipsum` (hardcoded in `server.c` and `client.c`)
* **Encrypted/Decrypted View:** To demonstrate the cipher, both terminals will display:
    1.  The **Encrypted** message (as a hex dump, to correctly show null bytes).
    2.  The **Decrypted** (real) message as plain text.

## How to Run

You will need two separate terminal windows to run this application: one for the server and one for the client.

### Prerequisites

* A C compiler (e.g., `gcc` on Linux/macOS)
* The `server.c` and `client.c` files.

---

### Step 1: Compile the Programs

Open a terminal, navigate to the directory where you saved the files, and run the following commands to compile both programs:

```bash
# Compile the server
gcc server.c -o server

# Compile the client
gcc client.c -o client
