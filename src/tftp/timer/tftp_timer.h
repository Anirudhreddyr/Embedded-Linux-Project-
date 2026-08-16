#ifndef TFTP_TIMER_H
#define TFTP_TIMER_H

#include <stdbool.h>
#include <stdint.h>

#include "tftp_error.h"

typedef struct tftp_timer tftp_timer_t;

typedef struct
{
    uint32_t timeout_ms;
    uint32_t max_retries;
} tftp_timer_config_t;

tftp_status_t tftp_timer_init(
    tftp_timer_t *self,
    const tftp_timer_config_t *config);

tftp_status_t tftp_timer_start(
    tftp_timer_t *self);

tftp_status_t tftp_timer_stop(
    tftp_timer_t *self);

tftp_status_t tftp_timer_expire(
    tftp_timer_t *self);

bool tftp_timer_is_running(
    const tftp_timer_t *self);

bool tftp_timer_is_expired(
    const tftp_timer_t *self);

uint32_t tftp_timer_retry_count(
    const tftp_timer_t *self);

bool tftp_timer_retry_available(
    const tftp_timer_t *self);

tftp_status_t tftp_timer_retry(
    tftp_timer_t *self);

#endif /* TFTP_TIMER_H */
