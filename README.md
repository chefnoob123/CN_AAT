AAT for Computer Networks Course for 25-26 Odd semester.


Simple Encrypted C Chat (Client-Server)

This is a basic client-server chat application written in C. It establishes a TCP connection between a single client and a server, allowing for full-duplex (two-way) communication.

All messages sent between the client and server are encrypted using a simple, symmetric XOR cipher with a hardcoded key.

Features

TCP Socket Communication: Uses standard Berkeley sockets for reliable, connection-oriented chat.

Full-Duplex Chat: The server uses the select() system call to monitor both the client socket and its own terminal (stdin) for input. This allows both the server operator and the client to type and send messages at any time without blocking each other.

XOR Encryption: All messages are encrypted/decrypted using a simple XOR cipher.

Key: navit (hardcoded in server.c and client.c)

Encrypted/Decrypted View: To demonstrate the cipher, both terminals will display:

The Encrypted message (as a hex dump, to correctly show null bytes).

The Decrypted (real) message as plain text.

How to Run

You will need two separate terminal windows to run this application: one for the server and one for the client.

Prerequisites

A C compiler (e.g., gcc on Linux/macOS)

The server.c and client.c files.

Step 1: Compile the Programs

Open a terminal, navigate to the directory where you saved the files, and run the following commands to compile both programs:

# Compile the server
gcc server.c -o server

# Compile the client
gcc client.c -o client


This will create two executable files: server and client.

Step 2: Run the Server (Terminal 1)

In your first terminal window, start the server. You must provide a port number for it to listen on.

Example:

./server 9999


The server will start and print: Server listening on port 9999...

Step 3: Run the Client (Terminal 2)

In a second terminal window, run the client. You must provide the hostname (e.g., localhost or 127.0.0.1 if on the same machine) and the same port number you gave the server.

Example:

./client localhost 9999


Step 4: Chat!

Once the client connects, you can start chatting.

Type a message in the client's terminal and press Enter.

Type a message in the server's terminal and press Enter.

In both windows, you will see the encrypted hex dump followed by the real, decrypted message, allowing you to have a full two-way conversation.

Step 5: Exit

To close the connection, type bye in either the client or server terminal and press Enter.

The client will exit.

The server will print Client disconnected. and will go back to listening for a new client.

To stop the server completely, go to its terminal and press Ctrl+C.
