#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#include "tftp_error.h"
#include "udp_transport.h"

typedef struct tftp_client tftp_client_t;

typedef struct
{
    udp_endpoint_t server;
} tftp_client_config_t;

tftp_status_t tftp_client_init(
    tftp_client_t *self,
    const tftp_client_config_t *config);

tftp_status_t tftp_client_start_rrq(
    tftp_client_t *self,
    const char *remote_filename);

tftp_status_t tftp_client_start_wrq(
    tftp_client_t *self,
    const char *remote_filename);

tftp_status_t tftp_client_process_event(
    tftp_client_t *self,
    const uint8_t *data,
    size_t length);

tftp_status_t tftp_client_abort(
    tftp_client_t *self);

bool tftp_client_is_active(
    const tftp_client_t *self);

#endif /* TFTP_CLIENT_H */
