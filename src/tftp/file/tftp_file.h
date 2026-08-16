#ifndef TFTP_FILE_H
#define TFTP_FILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tftp_error.h"

typedef enum
{
    TFTP_FILE_READ = 0,
    TFTP_FILE_WRITE
} tftp_file_mode_t;

typedef struct tftp_file tftp_file_t;

tftp_status_t tftp_file_init(
    tftp_file_t *self);

tftp_status_t tftp_file_open(
    tftp_file_t *self,
    const char *path,
    tftp_file_mode_t mode);

tftp_status_t tftp_file_read(
    tftp_file_t *self,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *bytes_read);

tftp_status_t tftp_file_write(
    tftp_file_t *self,
    const uint8_t *buffer,
    size_t length,
    size_t *bytes_written);

tftp_status_t tftp_file_close(
    tftp_file_t *self);

bool tftp_file_is_open(
    const tftp_file_t *self);

#endif /* TFTP_FILE_H */
