#ifndef TFTP_APP_H
#define TFTP_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "tftp_error.h"

typedef struct tftp_app tftp_app_t;

typedef struct
{
    uint16_t server_port;
    uint32_t max_transfers;
} tftp_app_config_t;

tftp_status_t tftp_app_init(
    tftp_app_t *self,
    const tftp_app_config_t *config);

tftp_status_t tftp_app_start(
    tftp_app_t *self);

tftp_status_t tftp_app_run(
    tftp_app_t *self);

tftp_status_t tftp_app_stop(
    tftp_app_t *self);

bool tftp_app_is_running(
    const tftp_app_t *self);

#endif /* TFTP_APP_H */
