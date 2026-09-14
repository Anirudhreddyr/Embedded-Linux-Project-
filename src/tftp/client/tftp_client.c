#include "tftp_client.h"

#include <stddef.h>
#include "tftp_transfer.h"

struct tftp_client
{
    bool initialized;
    bool running;

    reactor_t *reactor;
    udp_transport_t *transport;
    tftp_transfer_t *transfer;
};

static struct tftp_client g_client;
static bool g_client_in_use;

tftp_status_t tftp_client_acquire(
    tftp_client_t **out)
{
    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (g_client_in_use)
    {
        *out = NULL;
        return TFTP_ERR_NO_RESOURCE;
    }

    g_client_in_use = true;
    *out = &g_client;

    return TFTP_OK;
}

tftp_status_t tftp_client_init(
    tftp_client_t *self,
    const tftp_client_config_t *config)
{
    if ((NULL == self) || (NULL == config))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->reactor = config->reactor;
    self->transport = config->transport;
    self->transfer = NULL;

    self->initialized = true;
    self->running = false;

    return TFTP_OK;
}

tftp_status_t tftp_client_start(
    tftp_client_t *self)
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

tftp_status_t tftp_client_stop(
    tftp_client_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;

    return TFTP_OK;
}

tftp_status_t tftp_client_deinit(
    tftp_client_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;
    self->transfer = NULL;
    self->initialized = false;

    return TFTP_OK;
}

void tftp_client_release(
    tftp_client_t **self)
{
    if ((NULL != self) && (NULL != *self))
    {
        if (*self == &g_client)
        {
            g_client_in_use = false;
            *self = NULL;
        }
    }
}
