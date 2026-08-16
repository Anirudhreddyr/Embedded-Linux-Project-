#ifndef UDP_TRANSPORT_H
#define UDP_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <netinet/in.h>

#include "tftp_error.h"

typedef struct
{
    struct sockaddr_in address;
} udp_endpoint_t;

typedef struct udp_transport udp_transport_t;

typedef struct
{
    uint16_t local_port;
    bool non_blocking;
} udp_transport_config_t;

tftp_status_t udp_transport_init(
    udp_transport_t *self,
    const udp_transport_config_t *config);

tftp_status_t udp_transport_open(
    udp_transport_t *self);

tftp_status_t udp_transport_close(
    udp_transport_t *self);

tftp_status_t udp_transport_bind(
    udp_transport_t *self,
    uint16_t port);

tftp_status_t udp_transport_receive(
    udp_transport_t *self,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received,
    udp_endpoint_t *source);

tftp_status_t udp_transport_send(
    udp_transport_t *self,
    const uint8_t *buffer,
    size_t length,
    const udp_endpoint_t *destination);

int udp_transport_get_fd(
    const udp_transport_t *self);

#endif /* UDP_TRANSPORT_H */
