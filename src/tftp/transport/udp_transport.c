#include "udp_transport.h"

#include <fcntl.h>
#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>

struct udp_transport
{
    bool initialized;
    bool open;

    int fd;
    uint16_t local_port;
};

static struct udp_transport
    g_transport_pool[2];

static bool g_transport_used[2];

tftp_status_t udp_transport_acquire(
    udp_transport_t **out)
{
    uint32_t i;

    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    *out = NULL;

    for (i = 0U; i < 2U; ++i)
    {
        if (!g_transport_used[i])
        {
            g_transport_used[i] = true;
            *out = &g_transport_pool[i];

            return TFTP_OK;
        }
    }

    return TFTP_ERR_NO_RESOURCE;
}

tftp_status_t udp_transport_init(
    udp_transport_t *self,
    const udp_transport_config_t *config)
{
    int flags;

    if ((NULL == self) || (NULL == config))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->fd = socket(
        AF_INET,
        SOCK_DGRAM,
        0);

    if (self->fd < 0)
    {
        return TFTP_ERR_IO;
    }

    flags = fcntl(
        self->fd,
        F_GETFL,
        0);

    if (flags < 0)
    {
        (void)close(self->fd);
        self->fd = -1;

        return TFTP_ERR_IO;
    }

    if (fcntl(
            self->fd,
            F_SETFL,
            flags | O_NONBLOCK) < 0)
    {
        (void)close(self->fd);
        self->fd = -1;

        return TFTP_ERR_IO;
    }

    self->initialized = true;
    self->open = true;
    self->local_port = config->local_port;

    return TFTP_OK;
}

tftp_status_t udp_transport_close(
    udp_transport_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (self->open)
    {
        (void)close(self->fd);

        self->fd = -1;
        self->open = false;
    }

    return TFTP_OK;
}

tftp_status_t udp_transport_deinit(
    udp_transport_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    (void)udp_transport_close(self);

    self->initialized = false;

    return TFTP_OK;
}

void udp_transport_release(
    udp_transport_t **self)
{
    uint32_t i;

    if ((NULL == self) || (NULL == *self))
    {
        return;
    }

    for (i = 0U; i < 2U; ++i)
    {
        if (*self == &g_transport_pool[i])
        {
            g_transport_used[i] = false;
            *self = NULL;
            return;
        }
    }
}
