#ifndef SAFETY_PARAM_H
#define SAFETY_PARAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "urg_sensor.h"

typedef urg_t sensor_t;

enum {
    URG_FALSE = 0,
    URG_TRUE = 1,

    BUFFER_SIZE = 64 + 2 + 6,
    SAFETY_BUFFER_SIZE = 1024 * 9 + 512,

    EXPECTED_END = -1,

    RECEIVE_DATA_TIMEOUT,
    RECEIVE_DATA_COMPLETE,

    MAX_TIMEOUT = 140,
};

static const char RECEIVE_ERROR_MESSAGE[] = "receive error.";

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_PARAM_H */
