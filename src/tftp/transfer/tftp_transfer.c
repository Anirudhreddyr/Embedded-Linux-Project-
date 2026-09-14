#include "tftp_transfer.h"

#include <stddef.h>
#include <string.h>

#include "tftp_config.h"

struct tftp_transfer
{
    bool in_use;
    bool initialized;

    tftp_transfer_state_t state;

    tftp_opcode_t opcode;

    uint16_t block;
    uint16_t expected_block;

    uint32_t retry_count;

    udp_endpoint_t peer;

    char filename[TFTP_MAX_FILENAME_LENGTH + 1U];

    uint8_t packet_buffer[TFTP_MAX_PACKET_SIZE];
    size_t packet_length;

    tftp_file_t *file;
    tftp_timer_t *timer;
    udp_transport_t *transport;
};

static struct tftp_transfer
    g_transfer_pool[TFTP_MAX_TRANSFERS];

static void transfer_reset(tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return;
    }

    memset(self, 0, sizeof(*self));

    self->state = TFTP_TRANSFER_IDLE;
}

tftp_status_t tftp_transfer_acquire(
    tftp_transfer_t **out)
{
    uint32_t i;

    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    *out = NULL;

    for (i = 0U; i < TFTP_MAX_TRANSFERS; ++i)
    {
        if (!g_transfer_pool[i].in_use)
        {
            transfer_reset(&g_transfer_pool[i]);

            g_transfer_pool[i].in_use = true;

            *out = &g_transfer_pool[i];

            return TFTP_OK;
        }
    }

    return TFTP_ERR_NO_RESOURCE;
}

tftp_status_t tftp_transfer_init(
    tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->initialized = true;
    self->state = TFTP_TRANSFER_IDLE;
    self->block = 0U;
    self->expected_block = 0U;
    self->retry_count = 0U;
    self->packet_length = 0U;

    return TFTP_OK;
}

tftp_status_t tftp_transfer_start(
    tftp_transfer_t *self,
    const tftp_transfer_config_t *config)
{
    if ((NULL == self) || (NULL == config))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    self->opcode = config->opcode;
    self->peer = config->peer;

    if (NULL != config->filename)
    {
        strncpy(
            self->filename,
            config->filename,
            TFTP_MAX_FILENAME_LENGTH);

        self->filename[TFTP_MAX_FILENAME_LENGTH] = '\0';
    }
    else
    {
        self->filename[0] = '\0';
    }

    self->block = 0U;
    self->expected_block = 0U;
    self->retry_count = 0U;
    self->packet_length = 0U;

    switch (self->opcode)
    {
        case TFTP_OPCODE_RRQ:
            self->state = TFTP_TRANSFER_RRQ;
            break;

        case TFTP_OPCODE_WRQ:
            self->state = TFTP_TRANSFER_WRQ;
            break;

        default:
            self->state = TFTP_TRANSFER_ERROR;
            return TFTP_ERR_PROTOCOL;
    }

    return TFTP_OK;
}

tftp_status_t tftp_transfer_handle_event(
    tftp_transfer_t *self,
    tftp_transfer_event_t event,
    const uint8_t *data,
    size_t length)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    (void)data;
    (void)length;

    switch (event)
    {
        case TFTP_TRANSFER_EVENT_START:
            return tftp_transfer_process(self);

        case TFTP_TRANSFER_EVENT_RX_PACKET:
            /*
             * Packet processing will be implemented after the
             * packet encoder/decoder API is finalized.
             */
            break;

        case TFTP_TRANSFER_EVENT_TIMEOUT:
            /*
             * Timer/retransmission handling will be implemented
             * after transport and timer integration.
             */
            break;

        case TFTP_TRANSFER_EVENT_ERROR:
            self->state = TFTP_TRANSFER_ERROR;
            return TFTP_ERR_PROTOCOL;

        default:
            return TFTP_ERR_INVALID_ARGUMENT;
    }

    return TFTP_OK;
}

tftp_status_t tftp_transfer_process(
    tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    /*
     * Non-blocking state-machine execution.
     *
     * No recvfrom() loop.
     * No sleep().
     * No blocking file-transfer loop.
     *
     * Actual RRQ/WRQ packet processing will be added
     * after the packet and transport interfaces are finalized.
     */
    switch (self->state)
    {
        case TFTP_TRANSFER_IDLE:
            return TFTP_OK;

        case TFTP_TRANSFER_RRQ:
            /*
             * RRQ:
             * Open the requested file and prepare DATA block 1.
             */
            return TFTP_OK;

        case TFTP_TRANSFER_WRQ:
            /*
             * WRQ:
             * Open/create the destination file and prepare ACK 0.
             */
            return TFTP_OK;

        case TFTP_TRANSFER_WAIT_ACK:
            /*
             * Waiting for an ACK from the peer.
             */
            return TFTP_OK;

        case TFTP_TRANSFER_WAIT_DATA:
            /*
             * Waiting for a DATA packet from the peer.
             */
            return TFTP_OK;

        case TFTP_TRANSFER_COMPLETE:
            return TFTP_OK;

        case TFTP_TRANSFER_ERROR:
            return TFTP_ERR_INVALID_STATE;

        case TFTP_TRANSFER_CLEANUP:
            return TFTP_OK;

        default:
            return TFTP_ERR_INVALID_STATE;
    }
}

tftp_status_t tftp_transfer_abort(
    tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    self->state = TFTP_TRANSFER_ERROR;

    return TFTP_OK;
}

tftp_status_t tftp_transfer_cleanup(
    tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    transfer_reset(self);

    return TFTP_OK;
}

tftp_transfer_state_t tftp_transfer_get_state(
    const tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return TFTP_TRANSFER_ERROR;
    }

    return self->state;
}

uint16_t tftp_transfer_get_block(
    const tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return 0U;
    }

    return self->block;
}

bool tftp_transfer_is_complete(
    const tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return false;
    }

    return (TFTP_TRANSFER_COMPLETE == self->state);
}

void tftp_transfer_release(
    tftp_transfer_t **self)
{
    if ((NULL != self) && (NULL != *self))
    {
        (*self)->in_use = false;
        *self = NULL;
    }
}
