# Lightweight TFTP Implementation for Embedded Systems

## Description
A lightweight TFTP (Trivial File Transfer Protocol) server and client designed for embedded Linux and RTOS environments. 
Optimized for resource-constrained systems with a focus on low memory and CPU usage.

## Key Features
- Lightweight and optimized for embedded environments
- Supports RRQ (Read Request) and WRQ (Write Request)
- Implements DATA, ACK, and ERROR packet handling
- Compatible with both RTOS and Embedded Linux platforms
- UDP-based communication

## Technologies Used
- C Programming
- Embedded Linux
- RTOS
- UDP
- TFTP Protocol

## How It Works
The implementation follows the standard TFTP protocol workflow, handling file transfer through UDP sockets using a simple request–response model with error handling and retransmission support.

## Future Enhancements
- Add timeout and retransmission optimizations  
- Support for secure file transfer  
- Enhanced logging and debugging support
