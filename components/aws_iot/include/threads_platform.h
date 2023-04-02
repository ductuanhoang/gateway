#include "threads_interface.h"

#ifndef AWS_IOTSDK_THREADS_PLATFORM_H
#define AWS_IOTSDK_THREADS_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**
 * @brief Mutex Type
 *
 * definition of the Mutex   struct. Platform specific
 *
 */
struct _IoT_Mutex_t {
    SemaphoreHandle_t mutex;
};

#ifdef __cplusplus
}
#endif

#endif /* AWS_IOTSDK_THREADS_PLATFORM_H */


