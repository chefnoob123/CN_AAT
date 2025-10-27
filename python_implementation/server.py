import socket
from crypto_utils import encrypt, decrypt

HOST = '127.0.0.1'
PORT = 8080
KEY = 3

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((HOST, PORT))
s.listen(1)
print("Server waiting for connections...")

while True:
    conn, addr = s.accept()
    print(f"\nConnected by {addr}")

    while True:
        data = conn.recv(1024).decode()
        if not data:
            print("Client disconnected.")
            break

        # Show encrypted message from client
        print(f"Encrypted (received): {data}")

        # Decrypt and show plain text
        msg = decrypt(data, KEY)
        print(f"Decrypted (visible): {msg}")

        # Server replies
        reply = input("Server reply: ")
        enc_reply = encrypt(reply, KEY)

        # Show encryption for server reply
        print(f"Encrypted (sent): {enc_reply}")

        conn.sendall(enc_reply.encode())

    conn.close()
