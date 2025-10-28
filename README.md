# 🖧 CN_AAT — Computer Networks Assignment/Activity Tasks

> A set of practical implementations for the **Computer Networks (CN)** course during the **2025-26 Odd Semester**.

---

## 📚 Table of Contents
- [Overview](#overview)
- [Repository Structure](#repository-structure)
- [Features](#features)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Compilation & Running](#compilation--running)
- [Usage Examples](#usage-examples)
- [Contributing](#contributing)
- [License](#license)
- [Contact](#contact)

---

## 🧩 Overview

This repository contains code and documentation for **CN AAT (Assignment/Activity Tasks)** designed to help students understand fundamental concepts in **computer networking** — particularly **client-server communication**, **socket programming**, and **basic encryption**.

Two versions of the programs are included:
- **C Implementation** – for low-level network programming understanding.
- **Python Implementation** – for a higher-level, easily testable version.

---

## 🗂 Repository Structure

CN_AAT/
├── C_implementation/ # C-language client-server implementation
│ ├── server.c
│ └── client.c
│
├── python_implementation/ # Python implementation (socket programming)
│ ├── server.py
│ └── client.py
│
└── README.md # This documentation file


---

## ⚙️ Features

- **TCP Socket Communication**  
  Implements reliable, bidirectional communication between client and server.

- **Full-Duplex Chat**  
  Both endpoints can send and receive messages simultaneously.

- **Simple XOR Encryption**  
  Demonstrates basic symmetric encryption for educational purposes.

- **Encrypted & Decrypted Output**  
  Displays both encrypted (hex format) and decrypted (plain text) messages.

- **Cross-Language Examples**  
  Implementations in both **C** and **Python** to help compare low-level vs. high-level socket handling.

---

## 🚀 Getting Started

### 🔧 Prerequisites

Make sure you have the following installed:
- **C Compiler** — e.g. `gcc` (for C version)
- **Python 3.x** — for Python version
- **Terminal/Command Prompt** to run server & client simultaneously

---

### 🧱 Compilation & Running

#### 🖥️ C Implementation

1. Open **two terminals** — one for the **server** and one for the **client**.
2. Navigate to the `C_implementation/` directory.
3. Compile the programs:
   ```bash
   gcc server.c -o server
   gcc client.c -o client

    Start the server:

./server

In another terminal, start the client:

    ./client

    Type messages in either terminal — both should display the encrypted and decrypted forms.

🐍 Python Implementation

    Navigate to the python_implementation/ directory.

    Run the server:

python3 server.py

In another terminal, run the client:

    python3 client.py

    Begin sending messages between the two — encryption/decryption output will be shown in each console.

💬 Usage Examples

Example terminal output:

[Client] Enter message: Hello, Server!

Encrypted (hex): 48 21 35 60 73 ...
Decrypted: Hello, Server!

Both client and server will display messages in encrypted (hex) and decrypted (plain text) form, illustrating how simple XOR encryption works.
