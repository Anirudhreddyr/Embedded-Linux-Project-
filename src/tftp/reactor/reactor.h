#ifndef TFTP_REACTOR_H
#define TFTP_REACTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tftp_error.h"

typedef struct reactor reactor_t;

typedef enum
{
    REACTOR_EVENT_READ  = 1U << 0,
    REACTOR_EVENT_WRITE = 1U << 1,
    REACTOR_EVENT_ERROR = 1U << 2
} reactor_event_mask_t;

typedef void (*reactor_handler_t)(
    int fd,
    uint32_t events,
    void *context);

typedef struct
{
    int fd;
    uint32_t events;
    reactor_handler_t handler;
    void *context;
} reactor_registration_t;

typedef struct
{
    uint32_t max_handlers;
} reactor_config_t;

tftp_status_t reactor_init(
    reactor_t *self,
    const reactor_config_t *config);

tftp_status_t reactor_register(
    reactor_t *self,
    const reactor_registration_t *registration);

tftp_status_t reactor_unregister(
    reactor_t *self,
    int fd);

tftp_status_t reactor_run_once(
    reactor_t *self,
    int32_t timeout_ms);

tftp_status_t reactor_run(
    reactor_t *self);

tftp_status_t reactor_stop(
    reactor_t *self);

bool reactor_is_running(
    const reactor_t *self);

#endif /* TFTP_REACTOR_H */
