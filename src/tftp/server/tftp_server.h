#ifndef TFTP_SERVER_H
#define TFTP_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tftp_error.h"
#include "udp_transport.h"
#include "reactor.h"

typedef struct tftp_server tftp_server_t;

typedef struct
{
    uint16_t port;
    uint32_t max_transfers;
    reactor_t *reactor;
} tftp_server_config_t;

tftp_status_t tftp_server_acquire(tftp_server_t **out);
void tftp_server_release(tftp_server_t **self);

tftp_status_t tftp_server_init(
    tftp_server_t *self,
    const tftp_server_config_t *config);

tftp_status_t tftp_server_start(tftp_server_t *self);
tftp_status_t tftp_server_stop(tftp_server_t *self);
tftp_status_t tftp_server_deinit(tftp_server_t *self);

tftp_status_t tftp_server_process_event(
    tftp_server_t *self,
    const uint8_t *data,
    size_t length,
    const udp_endpoint_t *source);

bool tftp_server_is_running(const tftp_server_t *self);

#endif /* TFTP_SERVER_H */
