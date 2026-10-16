#include "tftp_server.h"

#include <stdio.h>
#include <string.h>

#include "tftp_config.h"

#define TFTP_SERVER_RX_BUFFER_SIZE (1500U)

struct tftp_server
{
    bool initialized;
    bool running;
    bool registered;

    uint16_t port;
    uint32_t max_transfers;

    reactor_t *reactor;
    udp_transport_t *transport;
};

static struct tftp_server g_server;
static bool g_server_in_use;

static void tftp_server_udp_handler(
    int fd,
    uint32_t events,
    void *context)
{
    tftp_server_t *self = (tftp_server_t *)context;
    uint8_t buffer[TFTP_SERVER_RX_BUFFER_SIZE];
    size_t received = 0U;
    udp_endpoint_t source;
    tftp_status_t status;

    if ((NULL == self) || (NULL == self->transport))
    {
        return;
    }

    if (0U != (events & REACTOR_EVENT_ERROR))
    {
        fprintf(stderr, "TFTP: UDP socket reported an error\n");
        return;
    }

    if (0U == (events & REACTOR_EVENT_READ))
    {
        return;
    }

    if (fd != udp_transport_get_fd(self->transport))
    {
        return;
    }

    status = udp_transport_receive(
        self->transport,
        buffer,
        sizeof(buffer),
        &received,
        &source);

    if (TFTP_OK != status)
    {
        if (TFTP_ERR_BUSY != status)
        {
            fprintf(stderr,
                    "TFTP: UDP receive failed: %d\n",
                    (int)status);
        }
        return;
    }

    printf("TFTP: received UDP datagram (%zu bytes)\n", received);
    fflush(stdout);

    status = tftp_server_process_event(
        self,
        buffer,
        received,
        &source);

    if (TFTP_OK != status)
    {
        fprintf(stderr,
                "TFTP: packet processing returned status %d\n",
                (int)status);
    }
}

tftp_status_t tftp_server_acquire(tftp_server_t **out)
{
    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    *out = NULL;

    if (g_server_in_use)
    {
        return TFTP_ERR_NO_RESOURCE;
    }

    memset(&g_server, 0, sizeof(g_server));
    g_server_in_use = true;
    *out = &g_server;

    return TFTP_OK;
}

void tftp_server_release(tftp_server_t **self)
{
    if ((NULL != self) && (NULL != *self))
    {
        if (*self == &g_server)
        {
            if (g_server.initialized)
            {
                (void)tftp_server_deinit(*self);
            }

            g_server_in_use = false;
            *self = NULL;
        }
    }
}

tftp_status_t tftp_server_init(
    tftp_server_t *self,
    const tftp_server_config_t *config)
{
    if ((NULL == self) || (NULL == config) ||
        (NULL == config->reactor))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (0U == config->port)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (self->initialized)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    self->port = config->port;
    self->max_transfers = config->max_transfers;

    if (0U == self->max_transfers ||
        self->max_transfers > TFTP_MAX_TRANSFERS)
    {
        self->max_transfers = TFTP_MAX_TRANSFERS;
    }

    self->reactor = config->reactor;
    self->transport = NULL;
    self->registered = false;
    self->running = false;
    self->initialized = true;

    return TFTP_OK;
}

tftp_status_t tftp_server_start(tftp_server_t *self)
{
    tftp_status_t status;
    udp_transport_config_t transport_config;
    reactor_registration_t registration;

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

    status = udp_transport_acquire(&self->transport);
    if (TFTP_OK != status)
    {
        return status;
    }

    transport_config.local_port = self->port;
    transport_config.non_blocking = true;

    status = udp_transport_init(self->transport, &transport_config);
    if (TFTP_OK != status)
    {
        goto fail;
    }

    status = udp_transport_open(self->transport);
    if (TFTP_OK != status)
    {
        goto fail;
    }

    status = udp_transport_bind(self->transport, self->port);
    if (TFTP_OK != status)
    {
        goto fail;
    }

    registration.fd = udp_transport_get_fd(self->transport);
    registration.events = REACTOR_EVENT_READ | REACTOR_EVENT_ERROR;
    registration.handler = tftp_server_udp_handler;
    registration.context = self;

    if (registration.fd < 0)
    {
        status = TFTP_ERR_INVALID_STATE;
        goto fail;
    }

    status = reactor_register(self->reactor, &registration);
    if (TFTP_OK != status)
    {
        goto fail;
    }

    self->registered = true;
    self->running = true;

    printf("TFTP: UDP server listening on port %u\n",
           (unsigned int)self->port);
    fflush(stdout);

    return TFTP_OK;

fail:
    if (NULL != self->transport)
    {
        (void)udp_transport_close(self->transport);
        (void)udp_transport_deinit(self->transport);
        udp_transport_release(&self->transport);
    }

    self->registered = false;
    self->running = false;

    return status;
}

tftp_status_t tftp_server_process_event(
    tftp_server_t *self,
    const uint8_t *data,
    size_t length,
    const udp_endpoint_t *source)
{
    if ((NULL == self) || (NULL == data) || (NULL == source))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->running)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    /*
     * Stage 9 receive-path test:
     * The datagram has reached the server.
     *
     * RRQ/WRQ parsing and transfer dispatch will be
     * implemented in the next stage.
     */
    (void)length;

    return TFTP_OK;
}

tftp_status_t tftp_server_stop(tftp_server_t *self)
{
    tftp_status_t result = TFTP_OK;
    int fd;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;

    if (NULL == self->transport)
    {
        return TFTP_OK;
    }

    fd = udp_transport_get_fd(self->transport);

    if (self->registered && (NULL != self->reactor) && (fd >= 0))
    {
        tftp_status_t status = reactor_unregister(self->reactor, fd);

        if (TFTP_OK != status)
        {
            result = status;
        }

        self->registered = false;
    }

    (void)udp_transport_close(self->transport);
    (void)udp_transport_deinit(self->transport);
    udp_transport_release(&self->transport);

    return result;
}

tftp_status_t tftp_server_deinit(tftp_server_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (NULL != self->transport)
    {
        (void)tftp_server_stop(self);
    }

    self->running = false;
    self->registered = false;
    self->reactor = NULL;
    self->initialized = false;

    return TFTP_OK;
}

bool tftp_server_is_running(const tftp_server_t *self)
{
    if (NULL == self)
    {
        return false;
    }

    return self->running;
}

