#ifndef AUTOXYQ_ERROR_H
#define AUTOXYQ_ERROR_H

// 用户态 API 错误码
typedef enum {
    AUTOXYQ_OK                    =  0,
    AUTOXYQ_ERR_DRIVER_NOT_FOUND  = -1,
    AUTOXYQ_ERR_DEVICE_NOT_READY  = -2,
    AUTOXYQ_ERR_IOCTL_FAILED      = -3,
    AUTOXYQ_ERR_INVALID_PARAM     = -4,
    AUTOXYQ_ERR_QUEUE_FULL        = -5,
    AUTOXYQ_ERR_TIMEOUT           = -6,
} autoxyq_error_t;

// 获取错误码的可读描述
const char* autoxyq_strerror(int err);

#endif // AUTOXYQ_ERROR_H
