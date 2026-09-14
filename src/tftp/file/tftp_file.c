#include "tftp_file.h"

#include <stdio.h>
#include <stddef.h>
#include "tftp_config.h"

struct tftp_file
{
    bool in_use;
    bool open;

    FILE *fp;
    tftp_file_mode_t mode;
};

static struct tftp_file
    g_file_pool[TFTP_MAX_TRANSFERS];

tftp_status_t tftp_file_acquire(
    tftp_file_t **out)
{
    uint32_t i;

    if (NULL == out)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    *out = NULL;

    for (i = 0U; i < TFTP_MAX_TRANSFERS; ++i)
    {
        if (!g_file_pool[i].in_use)
        {
            g_file_pool[i].in_use = true;
            g_file_pool[i].open = false;
            g_file_pool[i].fp = NULL;

            *out = &g_file_pool[i];

            return TFTP_OK;
        }
    }

    return TFTP_ERR_NO_RESOURCE;
}

tftp_status_t tftp_file_init(
    tftp_file_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->open = false;
    self->fp = NULL;

    return TFTP_OK;
}

tftp_status_t tftp_file_open(
    tftp_file_t *self,
    const char *path,
    tftp_file_mode_t mode)
{
    const char *file_mode;

    if ((NULL == self) || (NULL == path))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (self->open)
    {
        return TFTP_ERR_INVALID_STATE;
    }

    if (TFTP_FILE_READ == mode)
    {
        file_mode = "rb";
    }
    else if (TFTP_FILE_WRITE == mode)
    {
        file_mode = "wb";
    }
    else
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    self->fp = fopen(path, file_mode);

    if (NULL == self->fp)
    {
        return TFTP_ERR_FILE;
    }

    self->mode = mode;
    self->open = true;

    return TFTP_OK;
}

tftp_status_t tftp_file_read(
    tftp_file_t *self,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *bytes_read)
{
    size_t count;

    if ((NULL == self) ||
        (NULL == buffer) ||
        (NULL == bytes_read))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->open || (TFTP_FILE_READ != self->mode))
    {
        return TFTP_ERR_INVALID_STATE;
    }

    count = fread(
        buffer,
        1U,
        buffer_size,
        self->fp);

    *bytes_read = count;

    if (ferror(self->fp))
    {
        return TFTP_ERR_FILE;
    }

    return TFTP_OK;
}

tftp_status_t tftp_file_write(
    tftp_file_t *self,
    const uint8_t *buffer,
    size_t length,
    size_t *bytes_written)
{
    size_t count;

    if ((NULL == self) ||
        (NULL == buffer) ||
        (NULL == bytes_written))
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (!self->open || (TFTP_FILE_WRITE != self->mode))
    {
        return TFTP_ERR_INVALID_STATE;
    }

    count = fwrite(
        buffer,
        1U,
        length,
        self->fp);

    *bytes_written = count;

    if (count != length)
    {
        return TFTP_ERR_FILE;
    }

    return TFTP_OK;
}

tftp_status_t tftp_file_close(
    tftp_file_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    if (self->open)
    {
        if (0 != fclose(self->fp))
        {
            self->fp = NULL;
            self->open = false;

            return TFTP_ERR_FILE;
        }

        self->fp = NULL;
        self->open = false;
    }

    return TFTP_OK;
}

tftp_status_t tftp_file_deinit(
    tftp_file_t *self)
{
    if (NULL == self)
    {
        return TFTP_ERR_INVALID_ARGUMENT;
    }

    return tftp_file_close(self);
}

void tftp_file_release(
    tftp_file_t **self)
{
    if ((NULL != self) && (NULL != *self))
    {
        (*self)->in_use = false;
        *self = NULL;
    }
}
