#include "tftp_transfer.h"

#include <stddef.h>
#include <string.h>

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

static void transfer_reset(struct tftp_transfer *self)
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
    self->transport = config->transport;

    if (NULL != config->filename)
    {
        strncpy(
            self->filename,
            config->filename,
            TFTP_MAX_FILENAME_LENGTH);

        self->filename[TFTP_MAX_FILENAME_LENGTH] = '\0';
    }

    if (TFTP_OPCODE_RRQ == self->opcode)
    {
        self->state = TFTP_TRANSFER_RRQ_SEND;
    }
    else if (TFTP_OPCODE_WRQ == self->opcode)
    {
        self->state = TFTP_TRANSFER_WRQ_WAIT_ACK;
    }
    else
    {
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

    switch (self->state)
    {
        case TFTP_TRANSFER_RRQ_SEND:
            /*
             * Read file and send DATA block.
             */
            break;

        case TFTP_TRANSFER_RRQ_WAIT_ACK:
            /*
             * Validate ACK and advance block.
             */
            break;

        case TFTP_TRANSFER_WRQ_WAIT_ACK:
            /*
             * Validate initial ACK.
             */
            break;

        case TFTP_TRANSFER_WRQ_WAIT_DATA:
            /*
             * Validate DATA and write file.
             */
            break;

        case TFTP_TRANSFER_COMPLETE:
            return TFTP_ERR_INVALID_STATE;

        case TFTP_TRANSFER_ERROR:
            return TFTP_ERR_INVALID_STATE;

        default:
            return TFTP_ERR_INVALID_STATE;
    }

    (void)event;
    (void)data;
    (void)length;

    return TFTP_OK;
}

tftp_status_t tftp_transfer_process(
    tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    /*
     * Non-blocking state-machine execution.
     *
     * No recvfrom() loop.
     * No sleep().
     * No blocking file-transfer loop.
     */

    return TFTP_OK;
}

tftp_status_t tftp_transfer_abort(
    tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->state = TFTP_TRANSFER_ERROR;

    return TFTP_OK;
}

tftp_status_t tftp_transfer_deinit(
    tftp_transfer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    /*
     * Stage 9 integration:
     * stop timer
     * close file
     * clear protocol state
     */

    transfer_reset(self);

    return TFTP_OK;
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
