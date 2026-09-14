#include "tftp_app.h"

#include <stddef.h>

#include "reactor.h"
#include "tftp_server.h"
#include "tftp_client.h"

struct tftp_app
{
    bool initialized;
    bool running;

    reactor_t *reactor;
    tftp_server_t *server;
    tftp_client_t *client;
};

static struct tftp_app g_app;
static bool g_app_in_use;

tftp_status_t tftp_app_acquire(tftp_app_t **out)
{
    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (g_app_in_use)
    {
        *out = NULL;
        return TFTP_ERR_NO_RESOURCE;
    }

    g_app_in_use = true;
    *out = &g_app;

    return TFTP_OK;
}

tftp_status_t tftp_app_init(
    tftp_app_t *self,
    const tftp_app_config_t *config)
{
    (void)config;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->initialized = true;
    self->running = false;

    self->reactor = NULL;
    self->server = NULL;
    self->client = NULL;

    return TFTP_OK;
}

tftp_status_t tftp_app_start(tftp_app_t *self)
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

tftp_status_t tftp_app_run(tftp_app_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    if (!self->running)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    /*
     * Stage 9 integration point:
     * reactor_run(self->reactor);
     */

    return TFTP_OK;
}

tftp_status_t tftp_app_stop(tftp_app_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;

    return TFTP_OK;
}

tftp_status_t tftp_app_deinit(tftp_app_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;
    self->initialized = false;

    return TFTP_OK;
}

void tftp_app_release(tftp_app_t **self)
{
    if ((NULL != self) && (NULL != *self))
    {
        if (*self == &g_app)
        {
            g_app_in_use = false;
            *self = NULL;
        }
    }
}
