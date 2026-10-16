#include "udp_transport.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>

#include <sys/socket.h>
#include <unistd.h>


#define UDP_TRANSPORT_POOL_SIZE 2U


struct udp_transport
{
    bool initialized;
    bool open;

    int fd;
    uint16_t local_port;
    bool non_blocking;
};


static struct udp_transport
    g_transport_pool[UDP_TRANSPORT_POOL_SIZE];

static bool
    g_transport_used[UDP_TRANSPORT_POOL_SIZE];


tftp_status_t udp_transport_acquire(
    udp_transport_t **out)
{
    uint32_t i;

    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    *out = NULL;

    for (i = 0U; i < UDP_TRANSPORT_POOL_SIZE; ++i)
    {
        if (!g_transport_used[i])
        {
            g_transport_used[i] = true;

            memset(
                &g_transport_pool[i],
                0,
                sizeof(g_transport_pool[i]));

            g_transport_pool[i].fd = -1;

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
    if ((NULL == self) || (NULL == config))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (self->initialized)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    self->fd = -1;
    self->local_port = config->local_port;
    self->non_blocking = config->non_blocking;
    self->open = false;

    self->initialized = true;

    return TFTP_OK;
}


tftp_status_t udp_transport_open(
    udp_transport_t *self)
{
    int flags;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    if (self->open)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    self->fd = socket(
        AF_INET,
        SOCK_DGRAM,
        0);

    if (self->fd < 0)
    {
        self->fd = -1;
        return TFTP_ERR_IO;
    }

    if (self->non_blocking)
    {
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
    }

    self->open = true;

    return TFTP_OK;
}


tftp_status_t udp_transport_bind(
    udp_transport_t *self,
    uint16_t port)
{
    struct sockaddr_in address;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    if (!self->open)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    memset(
        &address,
        0,
        sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(
            self->fd,
            (struct sockaddr *)&address,
            sizeof(address)) < 0)
    {
        return TFTP_ERR_IO;
    }

    self->local_port = port;

    return TFTP_OK;
}


tftp_status_t udp_transport_close(
    udp_transport_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    if (!self->open)
    {
        return TFTP_OK;
    }

    if (close(self->fd) < 0)
    {
        self->fd = -1;
        self->open = false;

        return TFTP_ERR_IO;
    }

    self->fd = -1;
    self->open = false;

    return TFTP_OK;
}


tftp_status_t udp_transport_deinit(
    udp_transport_t *self)
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

    status = udp_transport_close(self);

    if ((TFTP_OK != status) &&
        (TFTP_ERR_NOT_INITIALIZED != status))
    {
        return status;
    }

    self->initialized = false;
    self->local_port = 0U;
    self->non_blocking = false;

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

    for (i = 0U; i < UDP_TRANSPORT_POOL_SIZE; ++i)
    {
        if (*self == &g_transport_pool[i])
        {
            if (g_transport_used[i])
            {
                (void)udp_transport_deinit(*self);
            }

            memset(
                &g_transport_pool[i],
                0,
                sizeof(g_transport_pool[i]));

            g_transport_pool[i].fd = -1;
            g_transport_used[i] = false;

            *self = NULL;

            return;
        }
    }
}


tftp_status_t udp_transport_receive(
    udp_transport_t *self,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received,
    udp_endpoint_t *source)
{
    socklen_t address_length;
    ssize_t result;

    if ((NULL == self) ||
        (NULL == buffer) ||
        (NULL == received) ||
        (NULL == source))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    *received = 0U;

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    if (!self->open)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    if (0U == buffer_size)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    memset(
        source,
        0,
        sizeof(*source));

    address_length = sizeof(source->address);

    result = recvfrom(
        self->fd,
        buffer,
        buffer_size,
        0,
        (struct sockaddr *)&source->address,
        &address_length);

    if (result < 0)
    {
        if ((EAGAIN == errno) ||
            (EWOULDBLOCK == errno))
        {
            return TFTP_ERR_BUSY;
        }

        return TFTP_ERR_IO;
    }

    *received = (size_t)result;

    return TFTP_OK;
}


tftp_status_t udp_transport_send(
    udp_transport_t *self,
    const uint8_t *buffer,
    size_t length,
    const udp_endpoint_t *destination)
{
    ssize_t result;

    if ((NULL == self) ||
        (NULL == buffer) ||
        (NULL == destination))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    if (!self->open)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    if (0U == length)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    result = sendto(
        self->fd,
        buffer,
        length,
        0,
        (const struct sockaddr *)&destination->address,
        sizeof(destination->address));

    if (result < 0)
    {
        if ((EAGAIN == errno) ||
            (EWOULDBLOCK == errno))
        {
            return TFTP_ERR_BUSY;
        }

        return TFTP_ERR_IO;
    }

    if ((size_t)result != length)
    {
        return TFTP_ERR_IO;
    }

    return TFTP_OK;
}


int udp_transport_get_fd(
    const udp_transport_t *self)
{
    if ((NULL == self) ||
        !self->initialized ||
        !self->open)
    {
        return -1;
    }

    return self->fd;
}
