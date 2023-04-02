#ifndef AWS_IOT_PLATFORM_H
#define AWS_IOT_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "timer_interface.h"

/**
 * definition of the Timer struct. Platform specific
 */
struct Timer {
    uint32_t start_ticks;
    uint32_t timeout_ticks;
    uint32_t last_polled_ticks;
};

#ifdef __cplusplus
}
#endif

#endif /* AWS_IOT_PLATFORM_H */
