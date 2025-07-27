# Lightweight TFTP Implementation for Emebedded Systems
##
## 📘 Overview

This project implements a **lightweight Trivial File Transfer Protocol (TFTP) client and server** designed for **embedded systems**, supporting both **RTOS** and **Embedded Linux** platforms. It aims to simplify **firmware updates**, **configuration management**, and **remote diagnostics** for constrained embedded environments.

---

## 🚀 Key Features

- ✅ Support for core TFTP operations: RRQ, WRQ, DATA, ACK, and ERROR.
- 🧠 Lightweight architecture optimized for embedded devices.
- 📦 Octet mode (binary transfer) for reliable file integrity.
- ⚙️ Works on both **RTOS** and **Embedded Linux** environments.
- 🔐 Rate limiting and access control for enhanced security.
- 🛠️ Simple configuration via file or command-line interface.
- 📄 Logging support for debugging and monitoring.

## 📂 Project Structure

```bash
tftp-embedded/
├── server/
│   └── tftp_server.c
├── client/
│   └── tftp_client.c
└── README.md
