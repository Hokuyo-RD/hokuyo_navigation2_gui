#ifndef SAFETY_SENSOR_SAFETYCMD_ENG_H
#define SAFETY_SENSOR_SAFETYCMD_ENG_H

/*
	List of functions to request the data in safety sensor.
	Operation is not guranteed for firmware versions that does not support the commands. 
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "safety_param.h"

/*
	\brief Connection
		
	Connects to sensor with the specified IP address making it possible to transmit commands.
	If IP address is not provided, connection is tried with the default IP address (192.168.0.10).
	This function must be called once before any other functions in the library.
	
	Supported versions 01.00.00 or above
	
	\param[in, out] sensor Sensor management information
	\param[in] argc Parameter for program execution
	\param[in] argv Parameter for program execution
	
	\retval 0 Connection successful
	\retval <0 Connection fail
*/
extern int safety_sensor_open_safetyCMD(sensor_t *sensor, int argc, char *argv[]);

/*
	\brief Disconnection

	Disconnects the connected sensor.
	This function should be called before terminating the program.

	Supported versions 01.00.00 or above
	
	\param[in, out] sensor Sensor management information
*/
extern void safety_sensor_close_safetyCMD(sensor_t *sensor);

/*
	\brief VR00 command for sensor model

	Transmits VR00 command to obtain the sensor model.

	Supported versions 01.00.00 or above
	
	\param[in, out] sensor Sensor management information

	\return String containing sensor model. 
*/
extern const char *safety_sensor_product_type_safetyCMD(sensor_t *sensor);

/*
	\brief VR00 command for firmware version
	
	Transmits VR00 command to obtain firmware version.

	Supported versions 01.00.00 or above
	
	\param[in, out] sensor Sensor mangement information
	
	\return Sting containing firmware version
*/
extern const char *safety_sensor_firmware_version_safetyCMD(sensor_t *sensor);

/*
	\brief VR00 command for serial number
	
	Transmits VR00 command to obtain serial number.

	Supported versions 01.00.00 or above
	
	\param[in, out] sensor Sensor mangement information
	
	\return Sting containing serial number
*/
extern const char *safety_sensor_serial_id_safetyCMD(sensor_t *sensor);

/*
	\brief AR00 command transmission

	Transmits AR00 command.
	Reception is perfored by the distance acquisition function.
	
	Supported versions 01.00.00 or above
	
	\param[in, out] sensor Sensor mangement information
	
	\retval 0 Transmission successful
	\retval <0 Transmission failed
*/
extern int safety_sensor_request_distance_handshaking_safetyCMD(sensor_t *sensor);

/*
	\brief Transmit AR00/AR02 to obtain distance data

	Receives distance data from the sensor.
	This is a common function for obtaining distance data using either AR00 or AR02 commands.
	
	Supported versions 01.00.00 or above
	
	\param[in, out] sensor Sensor mangement information
	\param[out] distance Distance data [mm]
	\param[out] safety_data Safety data

	\retval >0 Number of distance data acquired
	\retval <=0 Data acquisition fail
*/
extern int safety_sensor_get_distance_safetyCMD(sensor_t *sensor, long *distance, safety_data_t *safety_data);

/*
	\brief Transmits AR01 to obtain distance data

	Transmits AR01 command.
	Reception is perfored by the distance and intensity acquisition function.
	
	Supported versions 01.00.00 or above
	
	\param[in, out] sensor Sensor mangement information
	
	\retval 0 Transmission successful
	\retval <0 Transmission failed
*/
extern int safety_sensor_request_distance_intensity_handshaking_safetyCMD(sensor_t *sensor);

/*
	\brief Distance and intensity acquisition 

	Receives distance and intensity data from the sensor.
	This is a common function for obtaining distance and intensity data using either AR01 or AR04 commands.
	
	Supported versions 01.00.00 or above
	
	\param[in, out] Sensor mangement information
	\param[out] distance Distance data [mm]
	\param[out] intensity Intensity data 
	\param[out] safety_data Safety data

	\retval >0 Number of distance and intensity data acquired
	\retval <=0 Data acquisition fail
*/
extern int safety_sensor_get_distance_intensity_safetyCMD(sensor_t *sensor, long *distance, unsigned short *intensity, safety_data_t *safety_data);

/*
	\brief Transmits AR02 to obtain distance data in continuous mode

	Transmits AR02 command
	Reception is perfored by the distance acquisition function

	Supported versions 01.00.00 and 01.00.03 or above (Not supported in firmware version 01.00.02)
                                                                                                                                      
	\param[in, out] sensor Sensor mangagement information
                                                                                                                                      	
	\retval 0 Transmission successful
	\retval <0 Transmission failed
*/
extern int safety_sensor_request_distance_continuous_safetyCMD(sensor_t *sensor);

/*
	\brief Transmits AR03 to stop continuous data mode initiated by AR02 command
	
	Transmits AR03 command
	Stops the continuous data output initiated by AR02 command
	
	Supported versions 01.00.00 and 01.00.03 or above (Not supported in firmware version 01.00.02)
	
	\param[in, out] sensor Sensor mangagement information

	\retval 0 Transmission successful
	\retval <0 Transmission failed
*/
extern int safety_sensor_stop_measurement_distance_safetyCMD(sensor_t *sensor);

/*
	\brief Transmits AR04 to obtain distance and intensity data in continuous mode
	
	Transmits AR04 command
	Reception is perfored by the distance acquisition function
	
	Supported versions 01.00.00 and 01.00.03 or above (Not supported in firmware version 01.00.02)
	
	\param[in, out] sensor Sensor mangagement information
	
	\retval 0 Transmission successful
	\retval <0 Transmission failed
*/
extern int safety_sensor_request_distance_intensity_continuous_safetyCMD(sensor_t *sensor);

/*
	\brief Transmits AR05 to stop continuous data mode initiated by AR04 command

	Transmits AR05 command
	Stops the continuous data output initiated by AR04 command

	Supported versions 01.00.00 and 01.00.03 or above (Not supported in firmware version 01.00.02)

	\param[in, out] sensor Sensor mangagement information
	
	\retval 0 Transmission successful
	\retval <0 Transmission failed
*/
extern int safety_sensor_stop_measurement_distance_intensity_safetyCMD(sensor_t *sensor);

#ifdef __cplusplus
}
#endif

#endif /* !SAFETY_SENSOR_SAFETYCMD_ENG_H */
