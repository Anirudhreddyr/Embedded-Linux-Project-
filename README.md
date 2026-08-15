# Lightweight TFTP Implementation for Embedded Linux

## Description

A lightweight TFTP (Trivial File Transfer Protocol) server and client implemented in C for Embedded Linux systems.

The project is being developed using a modular, responsibility-driven embedded software architecture. The design separates TFTP packet handling, UDP transport, local file access, transfer management, timer/retry handling, error handling, and event dispatch.

The architecture is designed to support deterministic, testable, and maintainable software components while keeping the implementation suitable for resource-constrained embedded systems.

---

## Supported TFTP Operations

The implementation is designed around the standard TFTP operations:

- **RRQ** — Read Request
- **WRQ** — Write Request
- **DATA** — Data packet
- **ACK** — Acknowledgement
- **ERROR** — Error packet

Communication uses UDP transport.

---

## Architecture

The project follows a modular, responsibility-driven architecture.

The major responsibilities are separated into independently testable modules for event dispatch, TFTP transfer management, packet processing, UDP transport, local file access, timer/retry management, and error handling.

### Modular Architecture

```mermaid
flowchart TB

    APP["Application<br/>app"]

    REACTOR["Reactor<br/>Event Dispatcher"]

    SERVER["TFTP Server"]
    CLIENT["TFTP Client"]

    TRANSFER["Transfer Context<br/>RRQ / WRQ State"]

    PACKET["Packet<br/>Parse / Build / Validate"]
    TRANSPORT["UDP Transport"]
    FILE["Local File Access"]
    TIMER["Timer<br/>Timeout / Retry"]
    ERROR["Error<br/>TFTP Error Handling"]

    APP --> REACTOR

    REACTOR --> SERVER
    REACTOR --> CLIENT
    REACTOR --> TIMER

    SERVER --> TRANSFER
    CLIENT --> TRANSFER

    TRANSFER --> PACKET
    TRANSFER --> TRANSPORT
    TRANSFER --> FILE
    TRANSFER --> TIMER
    TRANSFER --> ERROR

    PACKET --> TRANSPORT
