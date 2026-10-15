#include "tftp_packet.h"

#include <string.h>

static uint16_t read_u16_be(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8U) |
                      (uint16_t)data[1]);
}

static void write_u16_be(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)(value & 0xFFU);
}

static bool is_octet_mode(const uint8_t *mode, size_t length)
{
    static const char expected[] = "octet";
    size_t i;

    if (length != (sizeof(expected) - 1U))
    {
        return false;
    }

    for (i = 0U; i < length; ++i)
    {
        uint8_t c = mode[i];

        if ((c >= (uint8_t)'A') && (c <= (uint8_t)'Z'))
        {
            c = (uint8_t)(c + ((uint8_t)'a' - (uint8_t)'A'));
        }

        if (c != (uint8_t)expected[i])
        {
            return false;
        }
    }

    return true;
}

tftp_status_t tftp_packet_parse(
    const uint8_t *buffer,
    size_t length,
    tftp_packet_t *packet)
{
    uint16_t raw_opcode;

    if ((buffer == NULL) || (packet == NULL))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (length < 2U)
    {
        return TFTP_ERR_INVALID_PACKET;
    }

    raw_opcode = read_u16_be(buffer);

    memset(packet, 0, sizeof(*packet));

    switch (raw_opcode)
    {
        case TFTP_OPCODE_RRQ:
        case TFTP_OPCODE_WRQ:
            if (length < 6U)
            {
                return TFTP_ERR_INVALID_PACKET;
            }

            packet->opcode = (tftp_opcode_t)raw_opcode;
            packet->payload = &buffer[2];
            packet->payload_length = length - 2U;
            break;

        case TFTP_OPCODE_DATA:
            if ((length < TFTP_PACKET_HEADER_SIZE) ||
                (length > TFTP_MAX_PACKET_SIZE))
            {
                return TFTP_ERR_INVALID_PACKET;
            }

            packet->opcode = TFTP_OPCODE_DATA;
            packet->block = read_u16_be(&buffer[2]);
            packet->payload = &buffer[4];
            packet->payload_length = length - 4U;
            break;

        case TFTP_OPCODE_ACK:
            if (length != TFTP_PACKET_HEADER_SIZE)
            {
                return TFTP_ERR_INVALID_PACKET;
            }

            packet->opcode = TFTP_OPCODE_ACK;
            packet->block = read_u16_be(&buffer[2]);
            break;

        case TFTP_OPCODE_ERROR:
            if ((length < 5U) ||
                (buffer[length - 1U] != '\0'))
            {
                return TFTP_ERR_INVALID_PACKET;
            }

            packet->opcode = TFTP_OPCODE_ERROR;
            packet->block = read_u16_be(&buffer[2]);
            packet->payload = &buffer[4];
            packet->payload_length = length - 4U;
            break;

        default:
            return TFTP_ERR_INVALID_OPCODE;
    }

    return TFTP_OK;
}

tftp_status_t tftp_packet_parse_request(
    const uint8_t *buffer,
    size_t length,
    tftp_opcode_t *opcode,
    tftp_request_t *request)
{
    size_t filename_start = 2U;
    size_t filename_length = 0U;
    size_t mode_start;
    size_t mode_length = 0U;
    size_t i;
    tftp_packet_t packet;
    tftp_status_t status;

    if ((buffer == NULL) || (opcode == NULL) || (request == NULL))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    status = tftp_packet_parse(buffer, length, &packet);
    if (status != TFTP_OK)
    {
        return status;
    }

    if ((packet.opcode != TFTP_OPCODE_RRQ) &&
        (packet.opcode != TFTP_OPCODE_WRQ))
    {
        return TFTP_ERR_INVALID_OPCODE;
    }

    /* Find the filename's terminating NUL. */
    for (i = filename_start; i < length; ++i)
    {
        if (buffer[i] == '\0')
        {
            break;
        }

        ++filename_length;
    }

    if ((i >= length) ||
        (filename_length == 0U) ||
        (filename_length >= sizeof(request->filename)))
    {
        return TFTP_ERR_INVALID_PACKET;
    }

    mode_start = i + 1U;

    /* Find the mode's terminating NUL. */
    for (i = mode_start; i < length; ++i)
    {
        if (buffer[i] == '\0')
        {
            break;
        }

        ++mode_length;
    }

    if ((i >= length) || (mode_length == 0U))
    {
        return TFTP_ERR_INVALID_PACKET;
    }

    /*
     * This implementation currently supports only the octet mode
     * and does not yet implement TFTP option negotiation.
     */
    if ((i + 1U) != length)
    {
        return TFTP_ERR_PROTOCOL;
    }

    if (!is_octet_mode(&buffer[mode_start], mode_length))
    {
        return TFTP_ERR_PROTOCOL;
    }

    memcpy(request->filename, &buffer[filename_start], filename_length);
    request->filename[filename_length] = '\0';
    request->mode = TFTP_MODE_OCTET;
    *opcode = packet.opcode;

    return TFTP_OK;
}

tftp_status_t tftp_packet_build_ack(
    uint16_t block,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *length)
{
    if ((buffer == NULL) || (length == NULL))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (buffer_size < 4U)
    {
        return TFTP_ERR_NO_RESOURCE;
    }

    write_u16_be(&buffer[0], (uint16_t)TFTP_OPCODE_ACK);
    write_u16_be(&buffer[2], block);

    *length = 4U;
    return TFTP_OK;
}

tftp_status_t tftp_packet_build_data(
    uint16_t block,
    const uint8_t *data,
    size_t data_length,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *length)
{
    if ((buffer == NULL) || (length == NULL) ||
        ((data == NULL) && (data_length != 0U)))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (data_length > TFTP_BLOCK_SIZE)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (buffer_size < (4U + data_length))
    {
        return TFTP_ERR_NO_RESOURCE;
    }

    write_u16_be(&buffer[0], (uint16_t)TFTP_OPCODE_DATA);
    write_u16_be(&buffer[2], block);

    if (data_length > 0U)
    {
        memcpy(&buffer[4], data, data_length);
    }

    *length = 4U + data_length;
    return TFTP_OK;
}

tftp_status_t tftp_packet_build_error(
    uint16_t error_code,
    const char *message,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *length)
{
    size_t message_length;
    size_t total_length;

    if ((message == NULL) || (buffer == NULL) || (length == NULL))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    message_length = strlen(message);
    total_length = 4U + message_length + 1U;

    if (buffer_size < total_length)
    {
        return TFTP_ERR_NO_RESOURCE;
    }

    write_u16_be(&buffer[0], (uint16_t)TFTP_OPCODE_ERROR);
    write_u16_be(&buffer[2], error_code);
    memcpy(&buffer[4], message, message_length);
    buffer[4U + message_length] = '\0';

    *length = total_length;
    return TFTP_OK;
}

bool tftp_packet_is_final_data(size_t payload_length)
{
    return payload_length < TFTP_BLOCK_SIZE;
}
