#ifndef PRIVATE_FUNCTIONS_H
#define PRIVATE_FUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "urg_sensor.h"

typedef struct
{
	void (*clear_urg_communication_buffer)(urg_t *urg, int timeout);
	void (*ignore_receive_data)(urg_t *urg, int timeout);
	int (*change_sensor_baudrate)(urg_t *urg, long current_baudrate, long next_baudrate);
	int (*set_errno_and_return)(urg_t *urg, int urg_errno);
	int (*safety_send_command)(urg_t *urg, const char *command);
	int (*safety_response)(urg_t *urg, const char* command, int timeout, char *receive_buffer, int receive_buffer_max_size);
	int (*safety_receive_data)(urg_t *urg, long data[], unsigned short intensity[], safety_data_t *safety_data);
} privateFuncPtr_t;

extern const privateFuncPtr_t privateFunctions;

#ifdef __cplusplus
}
#endif

#endif /* PRIVATE_FUNCTIONS_H */
