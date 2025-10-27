def encrypt(text, key=3):
    return ''.join(chr((ord(c) + key) % 256) for c in text)

def decrypt(text, key=3):
    return ''.join(chr((ord(c) - key) % 256) for c in text)
