#include "reactor.h"

#include <poll.h>
#include <stddef.h>

struct reactor_registration
{
    int fd;
    short events;
    reactor_event_callback_t callback;
    void *instance;
};

struct reactor
{
    bool initialized;
    bool running;

    struct reactor_registration
        registrations[TFTP_MAX_REACTOR_HANDLERS];

    uint32_t registration_count;
};

static struct reactor g_reactor;
static bool g_reactor_in_use;

tftp_status_t reactor_acquire(reactor_t **out)
{
    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (g_reactor_in_use)
    {
        *out = NULL;
        return TFTP_ERR_NO_RESOURCE;
    }

    g_reactor_in_use = true;
    *out = &g_reactor;

    return TFTP_OK;
}

tftp_status_t reactor_init(
    reactor_t *self,
    const reactor_config_t *config)
{
    (void)config;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->initialized = true;
    self->running = false;
    self->registration_count = 0U;

    return TFTP_OK;
}

tftp_status_t reactor_register(
    reactor_t *self,
    const reactor_registration_t *registration)
{
    if ((NULL == self) || (NULL == registration))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    if (self->registration_count >=
        TFTP_MAX_REACTOR_HANDLERS)
    {
        return TFTP_ERR_NO_RESOURCE;
    }

    self->registrations[self->registration_count].fd =
        registration->fd;

    self->registrations[self->registration_count].events =
        registration->events;

    self->registrations[self->registration_count].callback =
        registration->callback;

    self->registrations[self->registration_count].instance =
        registration->instance;

    self->registration_count++;

    return TFTP_OK;
}

tftp_status_t reactor_run_once(
    reactor_t *self,
    int32_t timeout_ms)
{
    struct pollfd poll_fds[TFTP_MAX_REACTOR_HANDLERS];
    uint32_t i;
    int result;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    for (i = 0U; i < self->registration_count; ++i)
    {
        poll_fds[i].fd =
            self->registrations[i].fd;

        poll_fds[i].events =
            self->registrations[i].events;

        poll_fds[i].revents = 0;
    }

    result = poll(
        poll_fds,
        self->registration_count,
        timeout_ms);

    if (result < 0)
    {
        return TFTP_ERR_IO;
    }

    if (0 == result)
    {
        return TFTP_ERR_TIMEOUT;
    }

    for (i = 0U; i < self->registration_count; ++i)
    {
        if (0 != poll_fds[i].revents)
        {
            if (NULL != self->registrations[i].callback)
            {
                self->registrations[i].callback(
                    self->registrations[i].instance,
                    poll_fds[i].revents);
            }
        }
    }

    return TFTP_OK;
}

tftp_status_t reactor_run(reactor_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = true;

    while (self->running)
    {
        (void)reactor_run_once(self, 100);
    }

    return TFTP_OK;
}

tftp_status_t reactor_stop(reactor_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;

    return TFTP_OK;
}

tftp_status_t reactor_deinit(reactor_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;
    self->registration_count = 0U;
    self->initialized = false;

    return TFTP_OK;
}

void reactor_release(reactor_t **self)
{
    if ((NULL != self) && (NULL != *self))
    {
        if (*self == &g_reactor)
        {
            g_reactor_in_use = false;
            *self = NULL;
        }
    }
}
