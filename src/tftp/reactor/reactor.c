#include "reactor.h"

#include <poll.h>
#include <stddef.h>

#define REACTOR_MAX_HANDLERS (16U)

typedef struct
{
    int fd;
    uint32_t events;
    reactor_handler_t handler;
    void *context;
} reactor_registration_internal_t;

struct reactor
{
    bool initialized;
    bool running;

    reactor_registration_internal_t
        registrations[REACTOR_MAX_HANDLERS];

    uint32_t registration_count;
    uint32_t max_handlers;
};

static struct reactor g_reactor;
static bool g_reactor_in_use;


/*
 * Convert our reactor event mask into poll() events.
 */
static short reactor_to_poll_events(uint32_t events)
{
    short poll_events = 0;

    if (0U != (events & REACTOR_EVENT_READ))
    {
        poll_events |= POLLIN;
    }

    if (0U != (events & REACTOR_EVENT_WRITE))
    {
        poll_events |= POLLOUT;
    }

    if (0U != (events & REACTOR_EVENT_ERROR))
    {
        poll_events |= POLLERR;
    }

    return poll_events;
}


/*
 * Convert poll() events into our reactor event mask.
 */
static uint32_t poll_to_reactor_events(short events)
{
    uint32_t reactor_events = 0U;

    if (0 != (events & POLLIN))
    {
        reactor_events |= REACTOR_EVENT_READ;
    }

    if (0 != (events & POLLOUT))
    {
        reactor_events |= REACTOR_EVENT_WRITE;
    }

    if (0 != (events & (POLLERR | POLLHUP | POLLNVAL)))
    {
        reactor_events |= REACTOR_EVENT_ERROR;
    }

    return reactor_events;
}


tftp_status_t reactor_init(
    reactor_t *self,
    const reactor_config_t *config)
{
    uint32_t max_handlers;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    max_handlers = REACTOR_MAX_HANDLERS;

    if (NULL != config)
    {
        if (0U != config->max_handlers)
        {
            max_handlers = config->max_handlers;
        }
    }

    if (max_handlers > REACTOR_MAX_HANDLERS)
    {
        max_handlers = REACTOR_MAX_HANDLERS;
    }

    self->initialized = true;
    self->running = false;
    self->registration_count = 0U;
    self->max_handlers = max_handlers;

    return TFTP_OK;
}


tftp_status_t reactor_register(
    reactor_t *self,
    const reactor_registration_t *registration)
{
    reactor_registration_internal_t *entry;

    if ((NULL == self) || (NULL == registration))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->initialized)
    {
        return TFTP_ERR_NOT_INITIALIZED;
    }

    if (registration->fd < 0)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (NULL == registration->handler)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (self->registration_count >= self->max_handlers)
    {
        return TFTP_ERR_NO_RESOURCE;
    }

    entry =
        &self->registrations[self->registration_count];

    entry->fd = registration->fd;
    entry->events = registration->events;
    entry->handler = registration->handler;
    entry->context = registration->context;

    self->registration_count++;

    return TFTP_OK;
}


tftp_status_t reactor_unregister(
    reactor_t *self,
    int fd)
{
    uint32_t i;

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
        if (self->registrations[i].fd == fd)
        {
            uint32_t j;

            for (j = i; j + 1U < self->registration_count; ++j)
            {
                self->registrations[j] =
                    self->registrations[j + 1U];
            }

            self->registration_count--;

            return TFTP_OK;
        }
    }

    return TFTP_ERR_INVALID_ARGUMENT;
}


tftp_status_t reactor_run_once(
    reactor_t *self,
    int32_t timeout_ms)
{
    struct pollfd poll_fds[REACTOR_MAX_HANDLERS];
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
            reactor_to_poll_events(
                self->registrations[i].events);

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
            uint32_t events;

            events =
                poll_to_reactor_events(
                    poll_fds[i].revents);

            if (0U != events)
            {
                self->registrations[i].handler(
                    self->registrations[i].fd,
                    events,
                    self->registrations[i].context);
            }
        }
    }

    return TFTP_OK;
}


tftp_status_t reactor_run(
    reactor_t *self)
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

    self->running = true;

    while (self->running)
    {
        status = reactor_run_once(self, 100);

        if ((status != TFTP_OK) &&
            (status != TFTP_ERR_TIMEOUT))
        {
            self->running = false;
            return status;
        }
    }

    return TFTP_OK;
}


tftp_status_t reactor_stop(
    reactor_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;

    return TFTP_OK;
}


bool reactor_is_running(
    const reactor_t *self)
{
    if (NULL == self)
    {
        return false;
    }

    return self->running;
}
