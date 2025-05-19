#include "safety_sensor_safetyCMD.h"
#include "privateInclude/private_functions.h"
#include "urg_sensor.h"
#include "urg_errno.h"
#include "urg_utils.h"
#include "safety_crc.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(URG_MSC)
#define snprintf _snprintf
#endif

typedef enum 
{
	SAFETY,
	SCIP,
	NOT_CONNECTED
} Protocol;

typedef enum
{
	CONTINUOUS_STOP_OK = 0,
	CONTINUOUS_STOP_ERROR = 1
} ContinuousStopResult;

const static privateFuncPtr_t *privateFunc = (const privateFuncPtr_t *)(&privateFunctions);

static int connect_safety_device(sensor_t *sensor, long baudrate);
static int open_sensor(sensor_t *sensor, urg_connection_type_t connection_type,
             const char *device_or_address, long baudrate_or_port);

// 接続
int safety_sensor_open_safetyCMD(sensor_t *sensor, int argc, char *argv[])
{
	// 接続初期設定
    urg_connection_type_t connection_type = URG_ETHERNET;
    long baudrate_or_port = 10940;
    const char *ip_address = "192.168.0.10";
	const char *device = ip_address;

	// 引数によるIPアドレス指定
	if (argc > 2)
	{
		printf("Too much argument.\n");
		return -1;
	}
	else if (argc == 2)
	{
		device = argv[1];
	}

	if (open_sensor(sensor, connection_type, device, baudrate_or_port) < 0) {
        printf("safety_open_sensor: %s, %ld: %s\n",
            device, baudrate_or_port, urg_error(sensor));
        return -1;
    }

	return 0;
}

// open
static int open_sensor(sensor_t *sensor, urg_connection_type_t connection_type,
             const char *device_or_address, long baudrate_or_port)
{
    int ret;
    long baudrate = baudrate_or_port;

    sensor->is_active = URG_FALSE;
    sensor->is_sending = URG_TRUE;
    sensor->last_errno = URG_NOT_CONNECTED;
    sensor->timeout = MAX_TIMEOUT;
    sensor->scanning_skip_scan = 0;
    sensor->error_handler = NULL;

	// CRCテーブル初期化
	safety_init_crc();

	// \~japanese デバイスへの接続
    // \~english Connects to the device
    ret = connection_open(&sensor->connection, connection_type,
                          device_or_address, baudrate_or_port);

    if (ret < 0) {
        switch (connection_type) {
        case URG_SERIAL:
            sensor->last_errno = URG_SERIAL_OPEN_ERROR;
            break;

        case URG_ETHERNET:
            sensor->last_errno = URG_ETHERNET_OPEN_ERROR;
            break;

        default:
            sensor->last_errno = URG_INVALID_RESPONSE;
            break;
        }
        return sensor->last_errno;
    }

    // \~japanese  指定したボーレートで URG と通信できるように調整
    // \~english Make adjustments so to connect with URG using the specified baudrate
    if (connection_type == URG_ETHERNET) {
        // \~japanese  Ethernet のときは仮の通信速度を指定しておく
        // \~english In case of Ethernet, sets a fake baudrate
        baudrate = 115200;
    }

	ret = connect_safety_device(sensor, baudrate);
    if (ret != URG_NO_ERROR) {
        return privateFunc->set_errno_and_return(sensor, ret);
	}
    sensor->is_sending = URG_FALSE;

    // \~japanese  変数の初期化
    // \~english Initializes variables
    sensor->last_errno = URG_NO_ERROR;
    sensor->range_data_byte = URG_COMMUNICATION_3_BYTE;
    sensor->specified_scan_times = 0;
    sensor->scanning_remain_times = 0;
    sensor->is_laser_on = URG_FALSE;

	sensor->is_active = URG_TRUE;

	// パラメータ設定
	// SCIPはPPで取得する
	sensor->min_distance = 0;
	sensor->max_distance = 40000;
	sensor->area_resolution = 1440;
	sensor->first_data_index = 0;
	sensor->last_data_index = 1080;
	sensor->last_data_index_fullstep = 2160;
	sensor->front_data_index = 540;

	return ret;
}

// \~japanese ボーレートを変更しながら接続する
// \~english Sets the baudrate and connects to the sensor
static int connect_safety_device(sensor_t *sensor, long baudrate)
{
    long try_baudrate[] = { 19200, 57600, 115200, 250000, 500000, 750000 };
    int try_times = sizeof(try_baudrate) / sizeof(try_baudrate[0]);
    int i;

    // \~japanese 指示されたボーレートから接続する
    // \~english Fixes the baudrate list to have the given value first
    for (i = 0; i < try_times; ++i) {
        if (try_baudrate[i] == baudrate) {
            try_baudrate[i] = try_baudrate[0];
            try_baudrate[0] = baudrate;
            break;
        }
    }

    for (i = 0; i < try_times; ++i) {
        enum
		{
			RECEIVE_BUFFER_SIZE = 200,
			STATUS_OFFSET = 10,
			STATUS_LENGTH = 2,
			RECEIVE_CHECK_OFFSET = 4,
			RECEIVE_CHECK_LENGTH = 6
		};
        char receive_buffer[RECEIVE_BUFFER_SIZE + 1];
        int ret;

        connection_set_baudrate(&sensor->connection, try_baudrate[i]);

        // \~japanese URGが動いているボーレート以外でコマンドを送信した場合にゴミが残る場合があるのでクリア87y7tre 
        // \~english Clear URG read buffer to avoid having garbage data resulting from the incorrect baudrate communication
        privateFunc->clear_urg_communication_buffer(sensor, MAX_TIMEOUT);

        // \~japanese VR を送信し、応答が返されるかでボーレートが一致しているかを確認する
		// \~english Sends the VR command and if the response is received then baudrate is correctly set
		ret = privateFunc->safety_response(sensor, "VR00", MAX_TIMEOUT, receive_buffer, RECEIVE_BUFFER_SIZE);

        if (ret <= 0) {
            if (ret == URG_INVALID_RESPONSE) {
                // \~japanese 異常なエコーバックのときは、距離データ受信中とみなして
                // \~japanese データを読み飛ばす
				// \~english If an invalid echoback is received, it is currently in measurement data transmission
				// \~english so skip the data
				privateFunc->ignore_receive_data(sensor, MAX_TIMEOUT);

                // \~japanese ボーレートを変更して戻る
				// \~english Changes the baudrate and returns
                return privateFunc->change_sensor_baudrate(sensor, try_baudrate[i], baudrate);

            } else {
                // \~japanese 応答がないときは、ボーレートを変更して、再度接続を行う
				// \~english If there is no response, changes the baudrate and re-connects
                privateFunc->ignore_receive_data(sensor, MAX_TIMEOUT);
                continue;
            }
        } else if (strncmp(&receive_buffer[RECEIVE_CHECK_OFFSET], "VR0000", RECEIVE_CHECK_LENGTH) == 0) {

            // \~japanese センサとホストのボーレートを変更して戻る
			// \~english Changes the baudrate and returns
            return privateFunc->change_sensor_baudrate(sensor, try_baudrate[i], baudrate);
        }
    }

    return privateFunc->set_errno_and_return(sensor, URG_NOT_DETECT_BAUDRATE_ERROR);
}

// 切断
void safety_sensor_close_safetyCMD(sensor_t *sensor)
{
    if (sensor->is_active)
	{
		// 垂れ流し停止処理
		safety_sensor_stop_measurement_distance_intensity_safetyCMD(sensor);
    }
    connection_close(&sensor->connection);
    sensor->is_active = URG_FALSE;
}

// VR00
// 製品情報
const char *safety_sensor_product_type_safetyCMD(sensor_t *sensor)
{
	return safety_sensor_product_type(sensor);
}

// ファームウェアバージョン
const char *safety_sensor_firmware_version_safetyCMD(sensor_t *sensor)
{
	return safety_sensor_firmware_version(sensor);
}

// シリアル番号
const char *safety_sensor_serial_id_safetyCMD(sensor_t *sensor)
{
	return safety_sensor_serial_id(sensor);
}

// AR00/AR01/AR02/AR04共通送信処理
static int send_AR(sensor_t *sensor, const char *command)
{
	enum {
		AR_LENGTH = 14
	};
	int send_length;

	// 送信処理
	send_length = privateFunc->safety_send_command(sensor, command);
	if (send_length != AR_LENGTH)
	{
		return privateFunc->set_errno_and_return(sensor, URG_SEND_ERROR);
	}

	return 0;
}

// AR00送信処理
int safety_sensor_request_distance_handshaking_safetyCMD(sensor_t *sensor)
{
	return send_AR(sensor, "AR00");
}

// AR00受信処理
int safety_sensor_get_distance_safetyCMD(sensor_t *sensor, long *distance, safety_data_t *safety_data)
{
	return safety_get_distance(sensor, distance, safety_data);
}

// AR01送信処理
int safety_sensor_request_distance_intensity_handshaking_safetyCMD(sensor_t *sensor)
{
	return send_AR(sensor, "AR01");
}

// AR01受信処理
int safety_sensor_get_distance_intensity_safetyCMD(sensor_t *sensor, long *distance, unsigned short *intensity, safety_data_t *safety_data)
{
	return safety_get_distance_intensity(sensor, distance, intensity, safety_data);
}

// AR02送信処理
int safety_sensor_request_distance_continuous_safetyCMD(sensor_t *sensor)
{
	return send_AR(sensor, "AR02");
}

// AR03/AR05共通コマンド送受信処理
static int stop_measurement_safetyCMD(sensor_t *sensor, const char *command)
{
	enum {
		AR_STOP_LENGTH = 14,
		// TCPウィンドウサイズは、デフォルトではWindowsが64k、Linuxが16k
		// AR02コマンドによる受信電文長は4379
		// よって、高々15回の受信でバッファあふれとなる
		// このため、バッファクリアには15回の読み捨てで十分となる
		MAX_READ_TO_CLEAR = 15,
		MAX_READ_TIMES = 5,
		MAX_SEND_STOP_COUNT = 3
	};
    int ret = URG_INVALID_RESPONSE;
	int send_length;
	int i;
	int read_count = 0;
	int send_stop_count;

	if (!sensor->is_active)
	{
		return privateFunc->set_errno_and_return(sensor, URG_NOT_CONNECTED);
	}
	
	// AR03/AR05を規定回数まで送信し、停止を試みる
	for (send_stop_count = 0; send_stop_count < MAX_SEND_STOP_COUNT; send_stop_count++)
	{
		// バッファ読み捨て
		do
		{
			ret = privateFunc->safety_receive_data(sensor, NULL, NULL, NULL);
			read_count++;
		} while ((ret != URG_NO_RESPONSE) && (read_count < MAX_READ_TO_CLEAR));

		// AR03 or AR05送信
		send_length = privateFunc->safety_send_command(sensor, command);
		if (send_length != AR_STOP_LENGTH)
		{
			return privateFunc->set_errno_and_return(sensor, URG_SEND_ERROR);
		}

		// AR03 or AR05の返信が来るまで、データを読み捨てる
		for (i = 0; i < MAX_READ_TIMES; i++)
		{
			ret = privateFunc->safety_receive_data(sensor, NULL, NULL, NULL);

			// AR03 or AR05応答
			if (ret == URG_NO_ERROR)
			{
				sensor->is_sending = URG_FALSE;
				return privateFunc->set_errno_and_return(sensor, URG_NO_ERROR);
			}

			// 受信データなし
			if (ret == URG_NO_RESPONSE)
			{
				break;
			}
		}
	}

	return ret;
}

// AR03送受信処理
int safety_sensor_stop_measurement_distance_safetyCMD(sensor_t *sensor)
{
	return stop_measurement_safetyCMD(sensor, "AR03");
}

// AR04送信処理
int safety_sensor_request_distance_intensity_continuous_safetyCMD(sensor_t *sensor)
{
	return send_AR(sensor, "AR04");
}

// AR05送受信処理
int safety_sensor_stop_measurement_distance_intensity_safetyCMD(sensor_t *sensor)
{
	return stop_measurement_safetyCMD(sensor, "AR05");
}

// AR06送信処理
int safety_sensor_request_fullstep_distance_handshaking_safetyCMD(sensor_t *sensor)
{
	return send_AR(sensor, "AR06");
}

// AR07送信処理
int safety_sensor_request_fullstep_distance_continuous_safetyCMD(sensor_t *sensor)
{
	return send_AR(sensor, "AR07");
}

// AR08送受信処理
int safety_sensor_stop_measurement_fullstep_distance_safetyCMD(sensor_t *sensor)
{
	return stop_measurement_safetyCMD(sensor, "AR08");
}

//XR00送信処理
int safety_sensor_request_status_safetyCMD(sensor_t *sensor)
{
	enum {
		XR_LENGTH = 14
	};
	int send_length;

	// 送信処理
	send_length = privateFunc->safety_send_command(sensor, "XR00");
	if (send_length != XR_LENGTH)
	{
		return privateFunc->set_errno_and_return(sensor, URG_SEND_ERROR);
	}

	return 0;
}

// XR00受信処理
int safety_sensor_get_status_safetyCMD(sensor_t *sensor, status_data_t *status_data)
{
	return safety_get_status(sensor, status_data);
}


// YR00 ~ YR08 共通送信処理
static int send_YR(sensor_t *sensor, const char *command)
{
	enum {
		YR_LENGTH = 26
	};
	int send_length;

	// 送信処理
	send_length = privateFunc->safety_send_command(sensor, command);
	if (send_length != YR_LENGTH)
	{
		return privateFunc->set_errno_and_return(sensor, URG_SEND_ERROR);
	}

	return 0;
}
//YR00 ~ YR08 送信処理
int safety_sensor_request_area_safetyCMD(sensor_t *sensor, area_cmd_param_t *area_cmd_param)
{
	enum{
		BUFFER_SIZE = 256,
		area_type_size = 2,
		area_number_size = 2,
		step_size = 4,
		group_size = 2,
	};
	int n;
	char header[] = "YR";
	char buffer[BUFFER_SIZE];

	snprintf(buffer, BUFFER_SIZE, "%s%0*X%0*X%0*X%0*X%0*X", header, area_type_size, area_cmd_param->area_type, 
																	area_number_size, area_cmd_param->area_number,
																	step_size, area_cmd_param->start_step,
																	step_size, area_cmd_param->end_step,
																	group_size, area_cmd_param->grouping);

	n = send_YR(sensor, buffer);		
}


// YR受信処理
int safety_sensor_get_area_safetyCMD(sensor_t *sensor, long *area)
{
	return safety_get_area(sensor, area);
}

// DL送信処理
int safety_sensor_request_detection_log_safetyCMD(sensor_t *sensor)
{
	enum {
		DL_LENGTH = 14
	};
	int send_length;

	// 送信処理
	send_length = privateFunc->safety_send_command(sensor, "DL00");
	if (send_length != DL_LENGTH)
	{
		return privateFunc->set_errno_and_return(sensor, URG_SEND_ERROR);
	}

	return 0;
}

// DL受信処理
int safety_sensor_get_detection_log_safetyCMD(sensor_t *sensor, detection_log_data_t *detection_log_data)
{
	return safety_get_detection_log(sensor, detection_log_data);
}

// DC送信処理
int safety_sensor_request_detection_log_clear_safetyCMD(sensor_t *sensor)
{
	enum {
		DC_LENGTH = 14
	};
	int send_length;

	// 送信処理
	send_length = privateFunc->safety_send_command(sensor, "DC00");
	if (send_length != DC_LENGTH)
	{
		return privateFunc->set_errno_and_return(sensor, URG_SEND_ERROR);
	}

	return 0;
}

// DC受信処理
int safety_sensor_get_detection_log_clear_safetyCMD(sensor_t *sensor)
{
	return safety_get_detection_log_clear(sensor);
}