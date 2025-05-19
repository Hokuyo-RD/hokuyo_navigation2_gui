/*
	Sample program to obtain distance and intensity data from the sensor using AR04 Command.
	Data is continuously recevied unltil "CTRL+C" is pressed.
		
	Normal execution (default connection)
	> get_distance_intensity_continuous_safetyCMD.exe
	
	Execution with spcific IP address
	> get_distance_intensity_continuous_safetyCMD.exe 192.168.0.11
	
	�v���R�}���h�ɂ��Z���T���狗���l�E���x���l���擾����T���v���v���O�����iAR04�R�}���h�g�p�j
	AR04�R�}���h���g�p���A�L�[�{�[�h����"CTRL+C"�����͂����܂ŋ����l�E���x���l�̎擾��A���ōs���܂�
	�I������AR05�R�}���h���g�p���A���ꗬ�����[�h���~���܂�

	�ʏ���s�i�f�t�H���g�ڑ���j
	> get_distance_intensity_continuous_safetyCMD.exe

	IP�A�h���X�w����s
	> get_distance_intensity_continuous_safetyCMD.exe 192.168.0.11
*/

#include "safety_sensor_safetyCMD.h"
#include <stdio.h>
#include <signal.h>

#define TRUE (1)
#define FALSE (0)
#define ALL_OUTPUT

/*
	Data reception complete flag.
	�f�[�^�擾�I���t���O

	FALSE : Data reception in progress 
		     �f�[�^�擾�p��
	TRUE  : Data reception complete
			�f�[�^�擾�I��
*/
static unsigned char dataReceiveEndFlag = FALSE;

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
	char encoder_input_pattern[1 + 1] = { 0 };
	char encoder_angular_velocity[4 + 1] = { 0 };
	char detection_start_step_p1[4 + 1] = { 0 };
	char detection_end_step_p1[4 + 1] = { 0 };
	char detection_start_step_p2[4 + 1] = { 0 };
	char detection_end_step_p2[4 + 1] = { 0 };
	char detection_start_step_w1[4 + 1] = { 0 };
	char detection_end_step_w1[4 + 1] = { 0 };
	char detection_start_step_w2[4 + 1] = { 0 };
	char detection_end_step_w2[4 + 1] = { 0 };

	// Convert the obtained information
	// �e����ϊ�
	
	// Area is specified by numbers 0 to 31. It is matched to 7 seg display number by adding 1.
	// �G���A��0�`31�œd���쐬����邽�߁A�o�͎���+1���邱�Ƃ�7seg�\���ƍ��킹��
	sprintf(area_num, "%d", safety_data.area_number + 1);
	sprintf(error_code, "%x", safety_data.error_code);
	sprintf(encoder_velocity, "%x", safety_data.encoder_velocity);
	sprintf(encoder_input_pattern, "%lx", safety_data.encoder_input_pattern_num);
	sprintf(time_stamp, "%lx", safety_data.timestamp);
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
	\brief Display the distance and intensity
		  �����l���x���l�\��

	Displays the distance and intensity
	�����l�ƃ��x���l��\�����܂��B


	\param[in] distance  Distance data
						�����l�f�[�^
	\param[in] intensity Intensity data
						���x���l�f�[�^
	\param[in] data_num Total number of data
						  �����l�f�[�^��

*/
static void print_distance_intensity(long distance[], unsigned short intensity[], int data_num)
{
	enum
	{
		CENTER_STEP = 540
	};
	int i;

	// Display the distance and intensity
	// �����l�E���x���l�\��
#ifdef ALL_OUTPUT
	for (i = 0; i < data_num; i++)
	{
		printf("Step %4d : Distance : %6ld / Intensity : %6d\n", i, distance[i], intensity[i]);
	}
#else
	printf("Step %4d : Distance : %6ld / Intensity : %6d\n", CENTER_STEP, distance[CENTER_STEP], intensity[CENTER_STEP]);
#endif
}	

/*
	\brief 
	Precess for signal reception
	�V�O�i����M����

	Enables the data reception complete flag when SIGINT (Interrupt due to CTRL+C key) occurs.
	As the format for signal reception processing function is fixed, 
	it is not used but only the parameter is assigned.
	SIGINT�iCTRL+C�ɂ��L�[�{�[�h���荞�݁j�����ɂ��A�f�[�^�擾�I���t���O��L���ɂ��܂��B
	�V�O�i����M�����̊֐��t�H�[�}�b�g�͌��܂��Ă��邽�߁A�����ł͎g�p���܂��񂪈������w�肵�܂��B

	\param[in] signal_name Signal name
							�V�O�i����
*/
static void data_receive_end_handler(int signal_name)
{
	// To avoid the gcc warning
	// gcc��warning���
	(void)signal_name;

	dataReceiveEndFlag = TRUE;
}

/*
	\brief Signal setting
	�V�O�i���ݒ�

	Signal is specified to generate the interrupt when SIGINT (CTRL+C key) is detected.
	SIGINT�iCTRL+C�ɂ��L�[�{�[�h���荞�݁j�ɂ��A���荞�݂𔭐������邽�߃V�O�i����ݒ肵�܂��B

	\param[in] signal_name Signal name
							�V�O�i����

	\retval 0 Setting complete
			 �ݒ芮��
	\retval <0 Setting fail
			    �ݒ莸�s
*/
static int set_data_receive_end_signal(int signal_name)
{
	// Signal setting
	// �V�O�i���ݒ�
	if (signal(signal_name, data_receive_end_handler) == SIG_ERR)
	{
		// Signal setting error
		// �V�O�i���ݒ�G���[
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	enum
	{
		MAX_NO_RECEIVE_COUNT = 5,
		MAX_STEP_SIZE = 1081
	};
    sensor_t sensor;
	safety_data_t safety_data;
    long distance[MAX_STEP_SIZE];
	unsigned short intensity[MAX_STEP_SIZE];
    int n;
	unsigned long scan_count = 0;
	int receive_error_count = 0;

	// Connect
	// �ڑ�
    if (safety_sensor_open_safetyCMD(&sensor, argc, argv) < 0) {
        return 1;
    }

	// Signal setting
	// Obtain the signal genetated when "CTRL+C" key is pressed
	// �V�O�i���ݒ�
	// �L�[�{�[�h����"CTRL+C"���͂ɂ��V�O�i�����M
	if (set_data_receive_end_signal(SIGINT) < 0)
	{
		return 3;
	}

	// Transmit VR00 Command to obtain the serial number
	// VR00�R�}���h�ɂ��V���A���ԍ��擾
	printf("Sensor serial ID : %s\n\n", safety_sensor_serial_id_safetyCMD(&sensor));

	// Receive the data
	// Transmit AR04
	// �f�[�^�擾
	// AR04���M
	if (safety_sensor_request_distance_intensity_continuous_safetyCMD(&sensor) < 0)
	{
	    safety_sensor_close_safetyCMD(&sensor);
		return 2;
	}

	// Continue the data reception until user presses the "CTRL+C" key
	// or the data receptions continuously fails for the specified number of retry count
	// ���[�U�ɂ��A�L�[�{�[�h����"CTRL+C"�����͂����܂Ńf�[�^�擾���p��
	// ���邢�͋K��񐔘A���Ńf�[�^�擾�Ɏ��s����܂Ōp��
	do
	{
		if (receive_error_count > 0) {
			// Transmit AR04 again
			if (safety_sensor_request_distance_intensity_continuous_safetyCMD(&sensor) < 0)
			{
				safety_sensor_close_safetyCMD(&sensor);
				return 2;
			}
		}
		// AR04 reception
		// AR04��M
		n = safety_sensor_get_distance_intensity_safetyCMD(&sensor, distance, intensity, &safety_data);
		scan_count++;

		if (n == 0) {
			// Data is not available in the command
		}
		else if (n > 255) {
			// Display
			// �\��
			printf("[Scan : %ld]\n", scan_count);
			print_info(safety_data);
			print_distance_intensity(distance, intensity, n);
			printf("\n");

			receive_error_count = 0;
		}
		else if (n < 0) {
			printf("Communication error or invalid response: %x.\n", n);
			receive_error_count++;
		}
		else {
			printf("Sensor status error: %x.\n", n);
			receive_error_count++;
		}
	} while((dataReceiveEndFlag == FALSE) && (receive_error_count < MAX_NO_RECEIVE_COUNT));

	// Stop the data receptoin using AR05 command
	// AR05�R�}���h�ɂ�萂�ꗬ����~
	if (safety_sensor_stop_measurement_distance_intensity_safetyCMD(&sensor) == 0)
	{
		printf("Succeeded to stop continuous mode.\n");
	}
	else
	{
		printf("Failed to stop continuous mode.\n");
	}

	//Disconnect
	// �ؒf
    safety_sensor_close_safetyCMD(&sensor);

#if defined(URG_MSC)
    getchar();
#endif
    return 0;
}
