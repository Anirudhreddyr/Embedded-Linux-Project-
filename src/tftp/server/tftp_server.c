#include "tftp_server.h"

#include <stddef.h>

#include "tftp_config.h"
#include "tftp_transfer.h"

struct tftp_server
{
    bool initialized;
    bool running;

    uint16_t port;

    reactor_t *reactor;
    udp_transport_t *transport;

    tftp_transfer_t *transfers[TFTP_MAX_TRANSFERS];

    uint32_t active_transfers;
};

static struct tftp_server g_server;
static bool g_server_in_use;

static tftp_transfer_t *find_free_transfer(void)
{
    uint32_t i;

    for (i = 0U; i < TFTP_MAX_TRANSFERS; ++i)
    {
        if (NULL == g_server.transfers[i])
        {
            return NULL;
        }
    }

    return NULL;
}

tftp_status_t tftp_server_acquire(tftp_server_t **out)
{
    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (g_server_in_use)
    {
        *out = NULL;
        return TFTP_ERR_NO_RESOURCE;
    }

    g_server_in_use = true;
    *out = &g_server;

    return TFTP_OK;
}

tftp_status_t tftp_server_init(
    tftp_server_t *self,
    const tftp_server_config_t *config)
{
    uint32_t i;

    if ((NULL == self) || (NULL == config))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->port = config->port;
    self->reactor =NULL;
    self->transport = NULL;
    self->active_transfers = 0U;

    for (i = 0U; i < TFTP_MAX_TRANSFERS; ++i)
    {
        self->transfers[i] = NULL;
    }

    self->initialized = true;
    self->running = false;

    return TFTP_OK;
}

tftp_status_t tftp_server_start(tftp_server_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    self->running = true;

    return TFTP_OK;
}

tftp_status_t tftp_server_process_event(
    tftp_server_t *self,
    const uint8_t *data,
    size_t length,
    const udp_endpoint_t *source)
{
    if ((NULL == self) ||
        (NULL == data) ||
        (NULL == source))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->running)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    /*
     * Stage 9 integration point:
     *
     * 1. Parse opcode.
     * 2. For RRQ/WRQ acquire a transfer.
     * 3. Initialize transfer.
     * 4. Dispatch event.
     */

    (void)length;

    return TFTP_OK;
}

tftp_status_t tftp_server_stop(tftp_server_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;

    return TFTP_OK;
}

tftp_status_t tftp_server_deinit(tftp_server_t *self)
{
    uint32_t i;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;

    for (i = 0U; i < TFTP_MAX_TRANSFERS; ++i)
    {
        self->transfers[i] = NULL;
    }

    self->active_transfers = 0U;
    self->initialized = false;

    return TFTP_OK;
}

void tftp_server_release(tftp_server_t **self)
{
    if ((NULL != self) && (NULL != *self))
    {
        if (*self == &g_server)
        {
            g_server_in_use = false;
            *self = NULL;
        }
    }
}
