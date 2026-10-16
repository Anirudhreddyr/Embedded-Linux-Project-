#include "tftp_app.h"

#include <stddef.h>
#include <stdio.h>

#include "reactor.h"
#include "tftp_server.h"

struct tftp_app
{
    bool initialized;
    bool running;

    reactor_t *reactor;
    tftp_server_t *server;
};

static struct tftp_app g_app;
static bool g_app_in_use;

tftp_status_t tftp_app_acquire(tftp_app_t **out)
{
    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    *out = NULL;

    if (g_app_in_use)
    {
        return TFTP_ERR_NO_RESOURCE;
    }

    g_app_in_use = true;
    g_app.initialized = false;
    g_app.running = false;
    g_app.reactor = NULL;
    g_app.server = NULL;

    *out = &g_app;

    return TFTP_OK;
}

tftp_status_t tftp_app_init(
    tftp_app_t *self,
    const tftp_app_config_t *config)
{
    tftp_status_t status;
    reactor_config_t reactor_config;
    tftp_server_config_t server_config;

    if ((NULL == self) || (NULL == config))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if ((0U == config->server_port) || self->initialized)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    reactor_config.max_handlers = 16U;

    status = reactor_acquire(&self->reactor);
    if (TFTP_OK != status)
    {
        return status;
    }

    status = reactor_init(self->reactor, &reactor_config);
    if (TFTP_OK != status)
    {
        goto fail;
    }

    status = tftp_server_acquire(&self->server);
    if (TFTP_OK != status)
    {
        goto fail;
    }

    server_config.port = config->server_port;
    server_config.max_transfers = config->max_transfers;
    server_config.reactor = self->reactor;

    status = tftp_server_init(self->server, &server_config);
    if (TFTP_OK != status)
    {
        goto fail;
    }

    self->initialized = true;
    self->running = false;

    return TFTP_OK;

fail:
    if (NULL != self->server)
    {
        tftp_server_release(&self->server);
    }

    if (NULL != self->reactor)
    {
        (void)reactor_deinit(self->reactor);
        reactor_release(&self->reactor);
    }

    return status;
}

tftp_status_t tftp_app_start(tftp_app_t *self)
{
    tftp_status_t status;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    if (self->running)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    status = tftp_server_start(self->server);
    if (TFTP_OK != status)
    {
        return status;
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
     * This blocks until reactor_stop() is called or
     * the reactor encounters a non-timeout error.
     */
    return reactor_run(self->reactor);
}

tftp_status_t tftp_app_stop(tftp_app_t *self)
{
    tftp_status_t server_status;
    tftp_status_t reactor_status;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;

    server_status = TFTP_OK;
    reactor_status = TFTP_OK;

    if (NULL != self->server)
    {
        server_status = tftp_server_stop(self->server);
    }

    if (NULL != self->reactor)
    {
        reactor_status = reactor_stop(self->reactor);
    }

    if (TFTP_OK != server_status)
    {
        return server_status;
    }

    return reactor_status;
}

tftp_status_t tftp_app_deinit(tftp_app_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (NULL != self->server)
    {
        (void)tftp_server_stop(self->server);
        (void)tftp_server_deinit(self->server);
        tftp_server_release(&self->server);
    }

    if (NULL != self->reactor)
    {
        (void)reactor_stop(self->reactor);
        (void)reactor_deinit(self->reactor);
        reactor_release(&self->reactor);
    }

    self->running = false;
    self->initialized = false;

    return TFTP_OK;
}

bool tftp_app_is_running(const tftp_app_t *self)
{
    if (NULL == self)
    {
        return false;
    }

    return self->running;
}

void tftp_app_release(tftp_app_t **self)
{
    if ((NULL != self) && (NULL != *self))
    {
        if (*self == &g_app)
        {
            (void)tftp_app_deinit(*self);
            g_app_in_use = false;
            *self = NULL;
        }
    }
}

