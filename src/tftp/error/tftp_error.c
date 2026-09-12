#include "tftp_error.h"

const char *tftp_error_string(
    tftp_status_t status)
{
    switch (status)
    {
        case TFTP_OK:
            return "OK";

        case TFTP_ERR_INVALID_ARGUMENT:
            return "invalid argument";

        case TFTP_ERR_INVALID_STATE:
            return "invalid state";

        case TFTP_ERR_NOT_INITIALIZED:
            return "not initialized";

        case TFTP_ERR_NO_RESOURCE:
            return "no resource";

        case TFTP_ERR_PROTOCOL:
            return "protocol error";

        case TFTP_ERR_IO:
            return "I/O error";

        case TFTP_ERR_FILE:
            return "file error";

        case TFTP_ERR_TIMEOUT:
            return "timeout";

        case TFTP_ERR_RETRY_EXHAUSTED:
            return "retry exhausted";

        default:
            return "unknown error";
    }
}
