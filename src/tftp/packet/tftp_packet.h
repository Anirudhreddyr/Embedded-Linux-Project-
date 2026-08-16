#ifndef TFTP_PACKET_H
#define TFTP_PACKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tftp_error.h"

#define TFTP_BLOCK_SIZE        (512U)
#define TFTP_PACKET_HEADER_SIZE (4U)
#define TFTP_MAX_PACKET_SIZE   (516U)

typedef enum
{
    TFTP_OPCODE_RRQ   = 1U,
    TFTP_OPCODE_WRQ   = 2U,
    TFTP_OPCODE_DATA  = 3U,
    TFTP_OPCODE_ACK   = 4U,
    TFTP_OPCODE_ERROR = 5U
} tftp_opcode_t;

typedef enum
{
    TFTP_MODE_OCTET = 0
} tftp_mode_t;

typedef struct
{
    tftp_opcode_t opcode;
    uint16_t block;
    const uint8_t *payload;
    size_t payload_length;
} tftp_packet_t;

typedef struct
{
    char filename[256];
    tftp_mode_t mode;
} tftp_request_t;

typedef struct
{
    uint16_t error_code;
    const char *message;
} tftp_error_packet_t;

tftp_status_t tftp_packet_parse(
    const uint8_t *buffer,
    size_t length,
    tftp_packet_t *packet);

tftp_status_t tftp_packet_parse_request(
    const uint8_t *buffer,
    size_t length,
    tftp_opcode_t *opcode,
    tftp_request_t *request);

tftp_status_t tftp_packet_build_ack(
    uint16_t block,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *length);

tftp_status_t tftp_packet_build_data(
    uint16_t block,
    const uint8_t *data,
    size_t data_length,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *length);

tftp_status_t tftp_packet_build_error(
    uint16_t error_code,
    const char *message,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *length);

bool tftp_packet_is_final_data(size_t payload_length);

#endif /* TFTP_PACKET_H */
