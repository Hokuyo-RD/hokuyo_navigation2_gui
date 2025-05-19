/*
	Sample program to obtain status data from the sensor using XR00 Command.
		
	Normal execution (default connection)
	> get_status_safetyCMD.exe
	
	Execution with spcific IP address
	> get_status_safetyCMD.exe 192.168.0.11
	
	�v���R�}���h�ɂ��Z���T����X�e�[�^�X���擾����T���v���v���O�����iXR00�R�}���h�g�p�j
	
		�ʏ���s�i�f�t�H���g�ڑ���j
	> get_status_safetyCMD.exe

	IP�A�h���X�w����s
	> get_status_safetyCMD.exe 192.168.0.11
*/

#include "safety_sensor_safetyCMD.h"
#include <stdio.h>
#include <signal.h>

#define TRUE (1)
#define FALSE (0)
#define ALL_OUTPUT


/*
	\brief Display the status data
		   �X�e�[�^�X�f�[�^�\��

	Displays the status data
	�X�e�[�^�X�f�[�^��\�����܂��B

	\param[in] status_data Status data
						    �X�e�[�^�X�f�[�^
*/
static void print_info(status_data_t status_data)
{
	char area_num[2 + 1] = {0};
	char error_code[2 + 1] = {0};
	char encoder_velocity[4 + 1] = {0};
	char time_stamp[8 + 1] = {0};
	char encoder_input_pattern[1 + 1] = { 0 };
	char encoder_angular_velocity[4 + 1] = { 0 };

	// Convert the obtained information
	// �e����ϊ�
	
	// Area is specified by numbers 0 to 31. It is matched to 7 seg display number by adding 1.
	// �G���A��0�`31�œd���쐬����邽�߁A�o�͎���+1���邱�Ƃ�7seg�\���ƍ��킹��
	sprintf(area_num, "%d", status_data.area_number + 1);
	sprintf(error_code, "%x", status_data.error_code);
	sprintf(encoder_velocity, "%x", status_data.encoder_velocity);
	sprintf(time_stamp, "%lx", status_data.timestamp);
	sprintf(encoder_input_pattern, "%lx", status_data.encoder_input_pattern_num);
	sprintf(encoder_angular_velocity, "%lx", status_data.encoder_angular_velocity);

	// Dispaly the obtained information
	// �e����\��
#ifdef ALL_OUTPUT
	printf("Time Stamp              : %s\n", time_stamp);
	printf("Operating Mode          : %s\n", status_data.is_setting ? "Setting" : "Normal");
	printf("Area Number             : %s\n", area_num);
	printf("Error State             : %s\n", status_data.is_error_detected ? "Error is detected" : "No error");
	printf("Error Code              : %s\n", error_code);
	printf("Lockout State           : %s\n", status_data.is_lockout ? "Lockout" : "Normal");
	printf("OSSD 1 State            : %s\n", status_data.is_ossd1_on ? "Off(Detected)" : "On(Not detected)");
	printf("OSSD 2 State            : %s\n", status_data.is_ossd2_on ? "Off(Detected)" : "On(Not detected)");
	printf("Warning 1 State         : %s\n", status_data.is_warning1_on ? "Off(Detected)" : "On(Not detected)");
	printf("Warning 2 State         : %s\n", status_data.is_warning2_on ? "Off(Detected)" : "On(Not detected)");
	printf("OSSD 3 State            : %s\n", status_data.is_ossd3_on ? "Off(Detected)" : "On(Not detected)");
	printf("OSSD 4 State            : %s\n", status_data.is_ossd4_on ? "Off(Detected)" : "On(Not detected)");
	printf("Muting/override State 1 : %s\n", status_data.is_mut_over1_on ? "Active" : "Not Active");
	printf("Muting/override State 2 : %s\n", status_data.is_mut_over2_on ? "Active" : "Not Active");
	printf("Reset Request 1         : %s\n", status_data.is_reset_req1_on ? "On" : "Off");
	printf("Reset Request 2         : %s\n", status_data.is_reset_req2_on ? "On" : "Off");
	printf("Encoder Velocity           : %s\n", encoder_velocity);
	printf("Laser Emission State    : %s\n", status_data.is_laser_off ? "Off" : "On");
	printf("Slave1 OSSD 1 2 State   : %s\n", status_data.is_slave1_ossd12_on ? "On" : "Off");
	printf("Slave2 OSSD 1 2 State   : %s\n", status_data.is_slave2_ossd12_on ? "On" : "Off");
	printf("Slave3 OSSD 1 2 State   : %s\n", status_data.is_slave3_ossd12_on ? "On" : "Off");
	printf("Slave1 OSSD 3 4 State   : %s\n", status_data.is_slave1_ossd34_on ? "On" : "Off");
	printf("Slave2 OSSD 3 4 State   : %s\n", status_data.is_slave2_ossd34_on ? "On" : "Off");
	printf("Slave3 OSSD 3 4 State   : %s\n", status_data.is_slave3_ossd34_on ? "On" : "Off");
	printf("Slave1 Warning 1 State  : %s\n", status_data.is_slave1_warning1_on ? "On" : "Off");
	printf("Slave2 Warning 1 State  : %s\n", status_data.is_slave2_warning1_on ? "On" : "Off");
	printf("Slave3 Warning 1 State  : %s\n", status_data.is_slave3_warning1_on ? "On" : "Off");
	printf("Slave1 Warning 2 State  : %s\n", status_data.is_slave1_warning2_on ? "On" : "Off");
	printf("Slave2 Warning 2 State  : %s\n", status_data.is_slave2_warning2_on ? "On" : "Off");
	printf("Slave3 Warning 2 State  : %s\n", status_data.is_slave3_warning2_on ? "On" : "Off");
	printf("Slave1 Laser Emission State    : %s\n", status_data.is_slave1_warning2_on ? "Off" : "On");
	printf("Slave2 Laser Emission State    : %s\n", status_data.is_slave2_warning2_on ? "Off" : "On");
	printf("Slave3 Laser Emission State    : %s\n", status_data.is_slave3_warning2_on ? "Off" : "On");
	printf("Laser off State			: %s\n", status_data.is_laser_off ? "On(Laser is OFF)" : "Off(Laser is ON)");
	printf("Optical Window Contamination Warning : %s\n", status_data.is_optical_window_contamination_warning ? "On" : "Off");
	printf("Encoder Input Pattern Number : %s\n", encoder_input_pattern);
	printf("Encoder Angular Velocity : %s\n", encoder_angular_velocity);
#else
	printf("Time Stamp              : %s\n", time_stamp);
	printf("Area Number             : %s\n", area_num);
	printf("OSSD 1 State            : %s\n", status_data.is_ossd1_on ? "Off(Detected)" : "ON(Not detected)");
	printf("Laser Emission State    : %s\n", status_data.is_laser_off ? "Off" : "On");
#endif

	printf("\n");
}


int main(int argc, char *argv[])
{
    sensor_t sensor;
	status_data_t status_data;
    int n;

	// Connect
	// �ڑ�
	if (safety_sensor_open_safetyCMD(&sensor, argc, argv) < 0) {
        return 1;
    }

	// Transmit VR00 Command to obtain the serial number
	// VR00�R�}���h�ɂ��V���A���ԍ��擾
	printf("Sensor serial ID : %s\n\n", safety_sensor_serial_id_safetyCMD(&sensor));

	// Transmit XR00 command
	// XR00�R�}���h���M
	if (safety_sensor_request_status_safetyCMD(&sensor) < 0)
	{
	    safety_sensor_close_safetyCMD(&sensor);
		return 2;
	}

	// Receive the status data
	//�X�e�[�^�X��M
	n = safety_sensor_get_status_safetyCMD(&sensor,&status_data);

	if (n <= 0)
	{
		printf("safety_sensor_get_status_safetyCMD() error.\n");
	}
	else
	{
		// Display
		// �\��
		print_info(status_data);
	}
	
	// Disconnect
	// �ؒf
	safety_sensor_close_safetyCMD(&sensor);

#if defined(URG_MSC)
    getchar();
#endif
    return 0;
}
