#include "tftp_transfer.h"
#include "tftp_packet.h"
#include "tftp_error.h"


#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "tftp_config.h"

struct tftp_transfer
{
    bool in_use;
    bool initialized;
    bool registered;
    bool final_data;

    tftp_transfer_state_t state;
    tftp_opcode_t opcode;

    uint16_t block;
    uint16_t expected_block;
    uint32_t retry_count;

    udp_endpoint_t peer;
    char filename[TFTP_MAX_FILENAME_LENGTH + 1U];

    uint8_t packet_buffer[TFTP_MAX_PACKET_SIZE];
    size_t packet_length;

    tftp_file_t *file;
    tftp_timer_t *timer;
    udp_transport_t *transport;
    reactor_t *reactor;
};

static struct tftp_transfer
    g_transfer_pool[TFTP_MAX_TRANSFERS];

static void transfer_reset(tftp_transfer_t *self)
{
    if (NULL != self)
    {
        memset(self, 0, sizeof(*self));
        self->state = TFTP_TRANSFER_IDLE;
    }
}

static bool transfer_filename_valid(const char *name)
{
    if ((NULL == name) || ('\0' == name[0]))
        return false;

    if ((0 == strcmp(name, ".")) ||
        (0 == strcmp(name, "..")))
        return false;

    if ((NULL != strchr(name, '/')) ||
        (NULL != strchr(name, '\\')))
        return false;

    return true;
}

static bool transfer_same_peer(
    const udp_endpoint_t *a,
    const udp_endpoint_t *b)
{
    return
        (a->address.sin_family == b->address.sin_family) &&
        (a->address.sin_addr.s_addr == b->address.sin_addr.s_addr) &&
        (a->address.sin_port == b->address.sin_port);
}

static void transfer_udp_handler(
    int fd,
    uint32_t events,
    void *context);

static tftp_status_t transfer_send_next_data(
    tftp_transfer_t *self)
{
    uint8_t payload[TFTP_BLOCK_SIZE];
    size_t bytes_read = 0U;
    size_t packet_length = 0U;
    tftp_status_t status;

    status = tftp_file_read(
        self->file,
        payload,
        sizeof(payload),
        &bytes_read);

    if (TFTP_OK != status)
        return status;

    self->block = (uint16_t)(self->block + 1U);

    status = tftp_packet_build_data(
        self->block,
        payload,
        bytes_read,
        self->packet_buffer,
        sizeof(self->packet_buffer),
        &packet_length);

    if (TFTP_OK != status)
        return status;

    self->packet_length = packet_length;
    self->final_data = tftp_packet_is_final_data(bytes_read);
    self->expected_block = self->block;

    status = udp_transport_send(
        self->transport,
        self->packet_buffer,
        self->packet_length,
        &self->peer);

    if (TFTP_OK != status)
        return status;

    self->state = TFTP_TRANSFER_WAIT_ACK;

    printf(
        "TFTP: sent DATA block %u (%zu bytes)%s\n",
        (unsigned int)self->block,
        bytes_read,
        self->final_data ? " [final]" : "");

    fflush(stdout);

    return TFTP_OK;
}

tftp_status_t tftp_transfer_acquire(
    tftp_transfer_t **out)
{
    uint32_t i;

    if (NULL == out)
        return TFTP_ERR_INVALID_ARGUMENT;

    *out = NULL;

    for (i = 0U; i < TFTP_MAX_TRANSFERS; ++i)
    {
        if (!g_transfer_pool[i].in_use)
        {
            transfer_reset(&g_transfer_pool[i]);
            g_transfer_pool[i].in_use = true;
            *out = &g_transfer_pool[i];
            return TFTP_OK;
        }
    }

    return TFTP_ERR_NO_RESOURCE;
}

tftp_status_t tftp_transfer_init(tftp_transfer_t *self)
{
    if (NULL == self)
        return TFTP_ERR_INVALID_ARGUMENT;

    if (!self->in_use)
        return TFTP_ERR_INVALID_STATE;

    self->initialized = true;
    self->state = TFTP_TRANSFER_IDLE;

    return TFTP_OK;
}

tftp_status_t tftp_transfer_start(
    tftp_transfer_t *self,
    const tftp_transfer_config_t *config)
{
    if ((NULL == self) || (NULL == config))
        return TFTP_ERR_INVALID_ARGUMENT;

    if (!self->initialized)
        return TFTP_ERR_NOT_INITIALIZED;

    if ((NULL == config->reactor) ||
        !transfer_filename_valid(config->filename))
        return TFTP_ERR_INVALID_ARGUMENT;

    if (strlen(config->filename) > TFTP_MAX_FILENAME_LENGTH)
        return TFTP_ERR_INVALID_ARGUMENT;

    self->opcode = config->opcode;
    self->peer = config->peer;
    self->reactor = config->reactor;

    (void)snprintf(
        self->filename,
        sizeof(self->filename),
        "%s",
        config->filename);

    self->block = 0U;
    self->expected_block = 0U;
    self->packet_length = 0U;

    if (TFTP_OPCODE_RRQ == self->opcode)
    {
        self->state = TFTP_TRANSFER_RRQ;
        return TFTP_OK;
    }

    self->state = TFTP_TRANSFER_ERROR;
    return TFTP_ERR_PROTOCOL;
}


tftp_status_t tftp_transfer_process(tftp_transfer_t *self)
{
    char path[sizeof(self->filename) + sizeof("tftp_root/")];
    udp_transport_config_t transport_config;
    reactor_registration_t registration;
    tftp_status_t status;
    size_t error_length = 0U;

    if (NULL == self)
        return TFTP_ERR_INVALID_ARGUMENT;

    if (!self->initialized)
        return TFTP_ERR_NOT_INITIALIZED;

    if (TFTP_TRANSFER_RRQ != self->state)
        return TFTP_ERR_INVALID_STATE;

    (void)snprintf(
        path,
        sizeof(path),
        "tftp_root/%s",
        self->filename);

    /*
     * Step 1: Create the dedicated ephemeral UDP socket.
     * TFTP ERROR responses must originate from the transfer TID.
     */
    status = udp_transport_acquire(&self->transport);
    if (TFTP_OK != status)
        return status;

    transport_config.local_port = 0U;
    transport_config.non_blocking = true;

    status = udp_transport_init(
        self->transport,
        &transport_config);

    if (TFTP_OK != status)
        return status;

    status = udp_transport_open(self->transport);
    if (TFTP_OK != status)
        return status;

    /*
     * Step 2: Acquire and initialize the file object.
     */
    status = tftp_file_acquire(&self->file);
    if (TFTP_OK != status)
        return status;

    status = tftp_file_init(self->file);
    if (TFTP_OK != status)
        return status;

    /*
     * Step 3: Attempt to open the requested file.
     */
    status = tftp_file_open(
        self->file,
        path,
        TFTP_FILE_READ);

    if (TFTP_OK != status)
    {
        fprintf(
            stderr,
            "TFTP: cannot open requested file: %s\n",
            path);

        /*
         * Step 4: Build TFTP ERROR packet.
         */
        tftp_status_t error_status;

        error_status = tftp_packet_build_error(
            TFTP_ERROR_FILE_NOT_FOUND,
            "File not found",
            self->packet_buffer,
            sizeof(self->packet_buffer),
            &error_length);

        if (TFTP_OK != error_status)
        {
            fprintf(
                stderr,
                "TFTP: failed to build ERROR packet: %d\n",
                (int)error_status);

            return status;
        }

        /*
         * Step 5: Send ERROR from the ephemeral transfer socket.
         */
        error_status = udp_transport_send(
            self->transport,
            self->packet_buffer,
            error_length,
            &self->peer);

        if (TFTP_OK != error_status)
        {
            fprintf(
                stderr,
                "TFTP: failed to send ERROR packet: %d\n",
                (int)error_status);
        }
        else
        {
            printf(
                "TFTP: sent ERROR code %u for %s\n",
                (unsigned int)TFTP_ERROR_FILE_NOT_FOUND,
                self->filename);

            fflush(stdout);
        }

        /*
         * Return the original file-open error.
         * The server's transfer_error path releases resources.
         */
        return status;
    }

    /*
     * Step 6: Register the transfer socket with the reactor.
     */
    registration.fd = udp_transport_get_fd(self->transport);
    registration.events = REACTOR_EVENT_READ | REACTOR_EVENT_ERROR;
    registration.handler = transfer_udp_handler;
    registration.context = self;

    status = reactor_register(self->reactor, &registration);
    if (TFTP_OK != status)
        return status;

    self->registered = true;

    printf(
        "TFTP: RRQ opened %s, transfer fd=%d\n",
        path,
        registration.fd);

    /*
     * Step 7: Begin normal RRQ DATA transmission.
     */
    return transfer_send_next_data(self);
}

tftp_status_t tftp_transfer_handle_event(
    tftp_transfer_t *self,
    tftp_transfer_event_t event,
    const uint8_t *data,
    size_t length)
{
    tftp_packet_t packet;
    tftp_status_t status;

    if (NULL == self)
        return TFTP_ERR_INVALID_ARGUMENT;

    if (!self->initialized)
        return TFTP_ERR_NOT_INITIALIZED;

    if (TFTP_TRANSFER_EVENT_START == event)
        return tftp_transfer_process(self);

    if (TFTP_TRANSFER_EVENT_ERROR == event)
    {
        self->state = TFTP_TRANSFER_ERROR;
        return TFTP_ERR_PROTOCOL;
    }

    if (TFTP_TRANSFER_EVENT_TIMEOUT == event)
        return TFTP_ERR_TIMEOUT;

    if (TFTP_TRANSFER_EVENT_RX_PACKET != event)
        return TFTP_ERR_INVALID_ARGUMENT;

    if ((NULL == data) || (length < TFTP_PACKET_HEADER_SIZE))
        return TFTP_ERR_INVALID_PACKET;

    if (TFTP_TRANSFER_WAIT_ACK != self->state)
        return TFTP_ERR_INVALID_STATE;

    status = tftp_packet_parse(data, length, &packet);
    if (TFTP_OK != status)
        return status;

    if (TFTP_OPCODE_ACK != packet.opcode)
        return TFTP_ERR_PROTOCOL;

    if (packet.block != self->expected_block)
    {
        fprintf(
            stderr,
            "TFTP: unexpected ACK block %u, expected %u\n",
            (unsigned int)packet.block,
            (unsigned int)self->expected_block);

        return TFTP_ERR_INVALID_BLOCK;
    }

    printf("TFTP: received ACK block %u\n",
           (unsigned int)packet.block);

    if (self->final_data)
    {
        self->state = TFTP_TRANSFER_COMPLETE;
        return TFTP_OK;
    }

    return transfer_send_next_data(self);
}

static void transfer_udp_handler(
    int fd,
    uint32_t events,
    void *context)
{
    tftp_transfer_t *self = (tftp_transfer_t *)context;
    uint8_t buffer[TFTP_MAX_PACKET_SIZE];
    size_t received = 0U;
    udp_endpoint_t source;
    tftp_status_t status;

    if ((NULL == self) || (NULL == self->transport))
        return;

    if (fd != udp_transport_get_fd(self->transport))
        return;

    if (0U != (events & REACTOR_EVENT_ERROR))
    {
        (void)tftp_transfer_abort(self);
        return;
    }

    if (0U == (events & REACTOR_EVENT_READ))
        return;

    status = udp_transport_receive(
        self->transport,
        buffer,
        sizeof(buffer),
        &received,
        &source);

    if (TFTP_ERR_BUSY == status)
        return;

    if (TFTP_OK != status)
    {
        (void)tftp_transfer_abort(self);
        return;
    }

    /* Reject packets from an unexpected client endpoint. */
    if (!transfer_same_peer(&self->peer, &source))
    {
        fprintf(stderr, "TFTP: packet from unexpected peer\n");
        return;
    }

    status = tftp_transfer_handle_event(
        self,
        TFTP_TRANSFER_EVENT_RX_PACKET,
        buffer,
        received);

    if (TFTP_OK != status)
    {
        fprintf(stderr, "TFTP: transfer event failed: %d\n",
                (int)status);
        (void)tftp_transfer_abort(self);
    }

    if ((TFTP_TRANSFER_COMPLETE == self->state) ||
        (TFTP_TRANSFER_ERROR == self->state))
    {
        tftp_transfer_t *transfer = self;
        tftp_transfer_release(&transfer);
    }
}

tftp_status_t tftp_transfer_abort(tftp_transfer_t *self)
{
    if (NULL == self)
        return TFTP_ERR_INVALID_ARGUMENT;

    self->state = TFTP_TRANSFER_ERROR;
    return TFTP_OK;
}

tftp_status_t tftp_transfer_cleanup(tftp_transfer_t *self)
{
    if (NULL == self)
        return TFTP_ERR_INVALID_ARGUMENT;

    if (self->registered && (NULL != self->reactor) &&
        (NULL != self->transport))
    {
        (void)reactor_unregister(
            self->reactor,
            udp_transport_get_fd(self->transport));

        self->registered = false;
    }

    if (NULL != self->file)
    {
        (void)tftp_file_deinit(self->file);
        tftp_file_release(&self->file);
    }

    if (NULL != self->transport)
    {
        udp_transport_release(&self->transport);
    }

    transfer_reset(self);
    return TFTP_OK;
}

tftp_transfer_state_t tftp_transfer_get_state(
    const tftp_transfer_t *self)
{
    return (NULL == self) ?
        TFTP_TRANSFER_ERROR : self->state;
}

uint16_t tftp_transfer_get_block(const tftp_transfer_t *self)
{
    return (NULL == self) ? 0U : self->block;
}

bool tftp_transfer_is_complete(const tftp_transfer_t *self)
{
    return (NULL != self) &&
        (TFTP_TRANSFER_COMPLETE == self->state);
}

void tftp_transfer_release(tftp_transfer_t **self)
{
    if ((NULL != self) && (NULL != *self))
    {
        (void)tftp_transfer_cleanup(*self);
        *self = NULL;
    }
}

