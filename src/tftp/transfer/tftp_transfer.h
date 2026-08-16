#ifndef TFTP_TRANSFER_H
#define TFTP_TRANSFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tftp_error.h"
#include "tftp_packet.h"
#include "udp_transport.h"
#include "tftp_file.h"
#include "tftp_timer.h"

typedef enum
{
    TFTP_TRANSFER_IDLE = 0,
    TFTP_TRANSFER_RRQ,
    TFTP_TRANSFER_WRQ,
    TFTP_TRANSFER_WAIT_ACK,
    TFTP_TRANSFER_WAIT_DATA,
    TFTP_TRANSFER_COMPLETE,
    TFTP_TRANSFER_ERROR,
    TFTP_TRANSFER_CLEANUP
} tftp_transfer_state_t;

typedef struct tftp_transfer tftp_transfer_t;

typedef struct
{
    tftp_opcode_t opcode;
    const char *filename;
    udp_endpoint_t peer;
} tftp_transfer_config_t;

typedef enum
{
    TFTP_TRANSFER_EVENT_RX_PACKET = 0,
    TFTP_TRANSFER_EVENT_TIMEOUT,
    TFTP_TRANSFER_EVENT_START,
    TFTP_TRANSFER_EVENT_ERROR
} tftp_transfer_event_t;

tftp_status_t tftp_transfer_init(
    tftp_transfer_t *self);

tftp_status_t tftp_transfer_start(
    tftp_transfer_t *self,
    const tftp_transfer_config_t *config);

tftp_status_t tftp_transfer_handle_event(
    tftp_transfer_t *self,
    tftp_transfer_event_t event,
    const uint8_t *data,
    size_t length);

tftp_status_t tftp_transfer_process(
    tftp_transfer_t *self);

tftp_status_t tftp_transfer_abort(
    tftp_transfer_t *self);

tftp_status_t tftp_transfer_cleanup(
    tftp_transfer_t *self);

tftp_transfer_state_t tftp_transfer_get_state(
    const tftp_transfer_t *self);

uint16_t tftp_transfer_get_block(
    const tftp_transfer_t *self);

bool tftp_transfer_is_complete(
    const tftp_transfer_t *self);

#endif /* TFTP_TRANSFER_H */
