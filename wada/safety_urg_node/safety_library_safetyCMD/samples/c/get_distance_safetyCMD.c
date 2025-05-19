/*
	Sample program to obtain distance data from the sensor using AR00 Command.
		
	Normal execution (default connection)
	> get_distance_safetyCMD.exe
	
	Execution with spcific IP address
	> get_distance_safetyCMD.exe 192.168.0.11
	
	
	�v���R�}���h�ɂ��Z���T���狗���l���擾����T���v���v���O�����iAR00�R�}���h�g�p�j

	�ʏ���s�i�f�t�H���g�ڑ���j
	> get_distance_safetyCMD.exe

	IP�A�h���X�w����s
	> get_distance_safetyCMD.exe 192.168.0.11
*/

#include "safety_sensor_safetyCMD.h"
#include <stdio.h>

#define ALL_OUTPUT

/*
	\brief Display safety data 
	 ���S�f�[�^�\��

	Displays the safety data.
	���S�f�[�^��\�����܂��B

	\param[in] safety_data 	Safety Data
							���S�f�[�^
*/
static void print_info(safety_data_t safety_data)
{
	char area_num[2 + 1] = {0};
	char error_code[2 + 1] = {0};
	char encoder_velocity[4 + 1] = {0};
	char time_stamp[8 + 1] = {0};
	char encoder_input_pattern[1 + 1] = {0};
	char encoder_angular_velocity[4 + 1] = {0};
	char detection_start_step_p1[4 + 1] = {0};
	char detection_end_step_p1[4 + 1] = {0};
	char detection_start_step_p2[4 + 1] = {0};
	char detection_end_step_p2[4 + 1] = {0};
	char detection_start_step_w1[4 + 1] = {0};
	char detection_end_step_w1[4 + 1] = {0};
	char detection_start_step_w2[4 + 1] = {0};
	char detection_end_step_w2[4 + 1] = {0};

	// Convert the obtained information
	// �e����ϊ�
	
	// Area is specified by numbers 0 to 31. It is matched to 7 seg display number by adding 1.
	// �G���A��0�`31�œd���쐬����邽�߁A�o�͎���+1���邱�Ƃ�7seg�\���ƍ��킹��
	sprintf(area_num, "%d", safety_data.area_number + 1);
	sprintf(error_code, "%x", safety_data.error_code);
	sprintf(encoder_velocity, "%x", safety_data.encoder_velocity);
	sprintf(time_stamp, "%lx", safety_data.timestamp);
	sprintf(encoder_input_pattern, "%lx", safety_data.encoder_input_pattern_num);
	sprintf(encoder_angular_velocity, "%lx", safety_data.encoder_angular_velocity);
	sprintf(detection_start_step_p1, "%lx", safety_data.protection_z1_detect_start_step);
	sprintf(detection_end_step_p1, "%lx", safety_data.protection_z1_detect_end_step);
	sprintf(detection_start_step_p2, "%lx", safety_data.protection_z2_detect_start_step);
	sprintf(detection_end_step_p2, "%lx", safety_data.protection_z2_detect_end_step);
	sprintf(detection_start_step_w1, "%lx", safety_data.warning_z1_detect_start_step);
	sprintf(detection_end_step_w1, "%lx", safety_data.warning_z1_detect_end_step);
	sprintf(detection_start_step_w2, "%lx", safety_data.warning_z2_detect_start_step);
	sprintf(detection_end_step_w2, "%lx", safety_data.warning_z2_detect_end_step);

	// Dispaly the obtained information
	// �e����\��
#ifdef ALL_OUTPUT
	printf("Time Stamp              : %s\n", time_stamp);
	printf("Operating Mode          : %s\n", safety_data.is_setting ? "Setting" : "Normal");
	printf("Area Number             : %s\n", area_num);
	printf("Error State             : %s\n", safety_data.is_error_detected ? "Error is detected" : "No error");
	printf("Error Code              : %s\n", error_code);
	printf("Lockout State           : %s\n", safety_data.is_lockout ? "Lockout" : "Normal");
	printf("OSSD 1 State            : %s\n", safety_data.is_ossd1_on ? "Off(Detected)" : "On(Not detected)");
	printf("OSSD 2 State            : %s\n", safety_data.is_ossd2_on ? "Off(Detected)" : "On(Not detected)");
	printf("Warning 1 State         : %s\n", safety_data.is_warning1_on ? "Off(Detected)" : "On(Not detected)");
	printf("Warning 2 State         : %s\n", safety_data.is_warning2_on ? "Off(Detected)" : "On(Not detected)");
	printf("OSSD 3 State            : %s\n", safety_data.is_ossd3_on ? "Off(Detected)" : "On(Not detected)");
	printf("OSSD 4 State            : %s\n", safety_data.is_ossd4_on ? "Off(Detected)" : "On(Not detected)");
	printf("Muting/override State 1 : %s\n", safety_data.is_mut_over1_on ? "Active" : "Not Active");
	printf("Muting/override State 2 : %s\n", safety_data.is_mut_over2_on ? "Active" : "Not Active");
	printf("Reset Request 1         : %s\n", safety_data.is_reset_req1_on ? "On" : "Off");
	printf("Reset Request 2         : %s\n", safety_data.is_reset_req2_on ? "On" : "Off");
	printf("Encoder Velocity           : %s\n", encoder_velocity);
	printf("Laser off State			: %s\n", safety_data.is_laser_off ? "On(Laser is OFF)" : "Off(Laser is ON)");
	printf("Optical Window Contamination Warning : %s\n", safety_data.is_optical_window_contamination_warning ? "On" : "Off");
	printf("Encoder Input Pattern Number : %s\n", encoder_input_pattern);
	printf("Encoder Angular Velocity : %s\n", encoder_angular_velocity);
	printf("Protection Zone1 Detection Start Step : %s\n", detection_start_step_p1);
	printf("Protection Zone1 Detection End Step : %s\n", detection_end_step_p1);
	printf("Protection Zone2 Detection Start Step : %s\n", detection_start_step_p2);
	printf("Protection Zone2 Detection End Step : %s\n", detection_end_step_p2);
	printf("Warning Zone1 Detection Start Step : %s\n", detection_start_step_w1);
	printf("Warning Zone1 Detection End Step : %s\n", detection_end_step_w1);
	printf("Warning Zone2 Detection Start Step : %s\n", detection_start_step_w2);
	printf("Warning Zone2 Detection End Step : %s\n", detection_end_step_w2);
#else
	printf("Time Stamp              : %s\n", time_stamp);
	printf("Area Number             : %s\n", area_num);
	printf("OSSD 1 State            : %s\n", safety_data.is_ossd1_on ? "Off(Detected)" : "ON(Not detected)");
#endif

	printf("\n");
}

/*
	\brief Dispaly the distance 
	�����l�\��

	Displays the distance
	�����l��\�����܂��B

	\param[in] distance  Distance data
						�����l�f�[�^
	\param[in] data_num Total number of data
						  �����l�f�[�^��
*/
static void print_distance(long distance[], int data_num)
{
	enum
	{
		CENTER_STEP = 540
	};
	int i;

	// Display the distance
	// �����l�\��
#ifdef ALL_OUTPUT
	for (i = 0; i < data_num; i++)
	{
		printf("Step %4d : Distance : %6ld\n", i, distance[i]);
	}
#else
	// Display only the center step data.
	// �\���X�e�b�v�𒆉��X�e�b�v�݂̂Ɍ���
	printf("Step %4d : Distance : %6ld\n", CENTER_STEP, distance[CENTER_STEP]);
#endif
}	

int main(int argc, char *argv[])
{
	enum
	{
		MAX_STEP_SIZE = 1081
	};
    sensor_t sensor;
	safety_data_t safety_data;
    long distance[MAX_STEP_SIZE];
    int n;

	// Connect
	// �ڑ�
    if (safety_sensor_open_safetyCMD(&sensor, argc, argv) < 0) {
        return 1;
    }

	// Transmit VR00 Command to obtain the serial number
	// VR00�R�}���h�ɂ��V���A���ԍ��擾
	printf("Sensor serial ID : %s\n\n", safety_sensor_serial_id_safetyCMD(&sensor));

	// Transmit AR00 command
	// AR00�R�}���h���M
	if (safety_sensor_request_distance_handshaking_safetyCMD(&sensor) < 0)
	{
	    safety_sensor_close_safetyCMD(&sensor);
		return 2;
	}

	// Receive the data
	// �����l��M
	n = safety_sensor_get_distance_safetyCMD(&sensor, distance, &safety_data);

	if (n == 0) {
		// Data is not available in the command
	}
	else if (n > 255) {
		// Display
		// �\��
		print_info(safety_data);
		print_distance(distance, n);
	}
	else if (n < 0) {
		printf("Communication error or invalid response.\n");
	}
	else {
		printf("Sensor status error: %x.\n", n);
	}

	// Disconnect
	// �ؒf
	safety_sensor_close_safetyCMD(&sensor);

#if defined(URG_MSC)
    getchar();
#endif
    return 0;
}
