#define _POSIX_C_SOURCE 200809L
#include "tftp_timer.h"

#include <stddef.h>
#include <time.h>
#include "tftp_config.h"

struct tftp_timer
{
    bool in_use;
    bool running;
    bool expired;

    uint32_t timeout_ms;
    uint32_t retry_count;
    uint32_t max_retries;

    uint64_t deadline_ms;
};

static struct tftp_timer
    g_timer_pool[TFTP_MAX_TRANSFERS];

static uint64_t timer_now_ms(void)
{
    struct timespec ts;

    if (0 != clock_gettime(
            CLOCK_MONOTONIC,
            &ts))
    {
        return 0U;
    }

    return ((uint64_t)ts.tv_sec * 1000ULL) +
           ((uint64_t)ts.tv_nsec / 1000000ULL);
}

tftp_status_t tftp_timer_acquire(
    tftp_timer_t **out)
{
    uint32_t i;

    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    *out = NULL;

    for (i = 0U; i < TFTP_MAX_TRANSFERS; ++i)
    {
        if (!g_timer_pool[i].in_use)
        {
            g_timer_pool[i].in_use = true;
            g_timer_pool[i].running = false;
            g_timer_pool[i].expired = false;
            g_timer_pool[i].retry_count = 0U;

            *out = &g_timer_pool[i];

            return TFTP_OK;
        }
    }

    return TFTP_ERR_NO_RESOURCE;
}

tftp_status_t tftp_timer_init(
    tftp_timer_t *self,
    const tftp_timer_config_t *config)
{
    if ((NULL == self) || (NULL == config))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->timeout_ms = config->timeout_ms;
    self->max_retries = config->max_retries;
    self->retry_count = 0U;
    self->running = false;
    self->expired = false;
    self->deadline_ms = 0U;

    return TFTP_OK;
}

tftp_status_t tftp_timer_start(
    tftp_timer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->expired = false;
    self->running = true;

    self->deadline_ms =
        timer_now_ms() +
        (uint64_t)self->timeout_ms;

    return TFTP_OK;
}

tftp_status_t tftp_timer_stop(
    tftp_timer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;
    self->expired = false;

    return TFTP_OK;
}

tftp_status_t tftp_timer_expire(
    tftp_timer_t *self)
{
    uint64_t now;

    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->running)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    now = timer_now_ms();

    if (now < self->deadline_ms)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    self->expired = true;
    self->running = false;

    return TFTP_OK;
}

tftp_status_t tftp_timer_retry(
    tftp_timer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (self->retry_count >= self->max_retries)
    {
        return TFTP_ERR_RETRY_EXHAUSTED;
    }

    self->retry_count++;

    return tftp_timer_start(self);
}

bool tftp_timer_is_running(
    const tftp_timer_t *self)
{
    return (NULL != self) && self->running;
}

bool tftp_timer_is_expired(
    const tftp_timer_t *self)
{
    return (NULL != self) && self->expired;
}

bool tftp_timer_retry_available(
    const tftp_timer_t *self)
{
    if (NULL == self)
    {
        return false;
    }

    return self->retry_count < self->max_retries;
}

uint32_t tftp_timer_retry_count(
    const tftp_timer_t *self)
{
    if (NULL == self)
    {
        return 0U;
    }

    return self->retry_count;
}

tftp_status_t tftp_timer_deinit(
    tftp_timer_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->running = false;
    self->expired = false;
    self->retry_count = 0U;

    return TFTP_OK;
}

void tftp_timer_release(
    tftp_timer_t **self)
{
    if ((NULL != self) && (NULL != *self))
    {
        (*self)->in_use = false;
        *self = NULL;
    }
}
