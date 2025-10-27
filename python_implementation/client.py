import socket
from crypto_utils import encrypt, decrypt

HOST = '127.0.0.1'
PORT = 8080
KEY = 3

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))

while True:
    msg = input("You: ")
    if msg.lower() in ['exit', 'quit']:
        break

    # Encrypt before sending
    enc_msg = encrypt(msg, KEY)
    print(f"Encrypted (sent): {enc_msg}")

    s.sendall(enc_msg.encode())

    # Receive reply
    data = s.recv(1024).decode()

    # Show encrypted and decrypted form
    print(f"Encrypted (received): {data}")
    print(f"Decrypted (visible): {decrypt(data, KEY)}\n")

s.close()
