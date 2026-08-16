#ifndef TFTP_ERROR_H
#define TFTP_ERROR_H

#include <stdint.h>



/*ERROR Module : The error module is the common status vocabulary used by the other modules
 *
 *Responsibility
 *	- Common module status values
 *	- TFTP protocol error codes
 *	- Conversion of status to diagnostic text
 * */
typedef enum
{
    TFTP_OK = 0,

    TFTP_ERR_INVALID_ARGUMENT = -1,
    TFTP_ERR_NOT_INITIALIZED = -2,
    TFTP_ERR_BUSY = -3,
    TFTP_ERR_TIMEOUT = -4,
    TFTP_ERR_RETRY_EXHAUSTED = -5,
    TFTP_ERR_INVALID_PACKET = -6,
    TFTP_ERR_INVALID_OPCODE = -7,
    TFTP_ERR_INVALID_BLOCK = -8,
    TFTP_ERR_FILE = -9,
    TFTP_ERR_TRANSPORT = -10,
    TFTP_ERR_NO_RESOURCE = -11,
    TFTP_ERR_PROTOCOL = -12,
    TFTP_ERR_INTERNAL = -13
} tftp_status_t;

typedef enum
{
    TFTP_ERROR_NOT_DEFINED = 0,
    TFTP_ERROR_FILE_NOT_FOUND = 1,
    TFTP_ERROR_ACCESS_VIOLATION = 2,
    TFTP_ERROR_DISK_FULL = 3,
    TFTP_ERROR_ILLEGAL_OPERATION = 4,
    TFTP_ERROR_UNKNOWN_TRANSFER_ID = 5,
    TFTP_ERROR_FILE_EXISTS = 6,
    TFTP_ERROR_NO_SUCH_USER = 7
} tftp_error_code_t;

const char *tftp_error_string(tftp_status_t status);

#endif /* TFTP_ERROR_H */
