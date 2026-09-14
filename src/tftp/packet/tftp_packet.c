#include "tftp_packet.h"

#include <stddef.h>

static uint16_t read_u16_be(
    const uint8_t *data)
{
    return (uint16_t)(
        ((uint16_t)data[0] << 8U) |
        (uint16_t)data[1]);
}

static void write_u16_be(
    uint8_t *data,
    uint16_t value)
{
    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)(value & 0xFFU);
}

tftp_status_t tftp_packet_decode(
    const uint8_t *buffer,
    size_t length,
    tftp_packet_t *packet)
{
    uint16_t opcode;

    if ((NULL == buffer) || (NULL == packet))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (length < 2U)
    {
        return TFTP_ERR_PROTOCOL;
    }

    opcode = read_u16_be(buffer);

    packet->opcode = (tftp_opcode_t)opcode;

    switch (packet->opcode)
    {
        case TFTP_OPCODE_RRQ:
        case TFTP_OPCODE_WRQ:
            /*
             * Validate filename/mode fields.
             */
            break;

        case TFTP_OPCODE_DATA:
            if (length < 4U)
            {
                return TFTP_ERR_PROTOCOL;
            }

            packet->block = read_u16_be(&buffer[2]);
            packet->payload = &buffer[4];
            packet->payload_length = length - 4U;
            break;

        case TFTP_OPCODE_ACK:
            if (length != 4U)
            {
                return TFTP_ERR_PROTOCOL;
            }

            packet->block = read_u16_be(&buffer[2]);
            break;

        case TFTP_OPCODE_ERROR:
            if (length < 5U)
            {
              return TFTP_ERR_PROTOCOL;
    	    }

            packet->payload = &buffer[4];
    	    packet->payload_length = length - 4U;
            break;

        default:
            return TFTP_ERR_PROTOCOL;
    }

    return TFTP_OK;
}

tftp_status_t tftp_packet_encode_ack(
    uint8_t *buffer,
    size_t buffer_size,
    uint16_t block,
    size_t *length)
{
    if ((NULL == buffer) ||
        (NULL == length))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (buffer_size < 4U)
    {
        return TFTP_ERR_NO_RESOURCE;
    }

    write_u16_be(
        &buffer[0],
        TFTP_OPCODE_ACK);

    write_u16_be(
        &buffer[2],
        block);

    *length = 4U;

    return TFTP_OK;
}

tftp_status_t tftp_packet_encode_data(
    uint8_t *buffer,
    size_t buffer_size,
    uint16_t block,
    const uint8_t *data,
    size_t data_length,
    size_t *length)
{
    if ((NULL == buffer) ||
        (NULL == data) ||
        (NULL == length))
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

    write_u16_be(
        &buffer[0],
        TFTP_OPCODE_DATA);

    write_u16_be(
        &buffer[2],
        block);

    for (size_t i = 0U; i < data_length; ++i)
    {
        buffer[4U + i] = data[i];
    }

    *length = 4U + data_length;

    return TFTP_OK;
}
