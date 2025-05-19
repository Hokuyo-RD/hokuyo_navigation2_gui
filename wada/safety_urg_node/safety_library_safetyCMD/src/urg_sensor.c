/*!
  \file
  \~japanese
  \brief URG センサ制御
  \~english
  \brief
 
  $Id$

  \~japanese
  \todo Mx 計測中に他の Mx コマンドを送信したときに、適切に動作するようにする
  \~english
  \todo Run correctly when a Mx measurement command is in operation and a second Mx command is received
*/

#include "urg_sensor.h"
#include "urg_errno.h"
#include "urg_utils.h"
#include "safety_crc.h"
#include "privateInclude/private_functions.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(URG_MSC)
#define snprintf _snprintf
#endif


enum {
    URG_FALSE = 0,
    URG_TRUE = 1,

    BUFFER_SIZE = 64 + 2 + 6,
    SAFETY_BUFFER_SIZE = 1024 * 9 + 512,

    EXPECTED_END = -1,

    RECEIVE_DATA_TIMEOUT,
    RECEIVE_DATA_COMPLETE,      /*!< \~japanese データを正常に受信   \~english Data was received correctly */

    PP_RESPONSE_LINES = 10,
    VV_RESPONSE_LINES = 7,
    II_RESPONSE_LINES = 9,

    MAX_TIMEOUT = 140,
};


static const char NOT_CONNECTED_MESSAGE[] = "not connected.";
static const char RECEIVE_ERROR_MESSAGE[] = "receive error.";

static int safety_check_crc(const char* data, size_t length);
static int safety_response(urg_t *urg, const char* command,
                         int timeout,
                         char *receive_buffer, int receive_buffer_max_size);
static int safety_send_command(urg_t *urg, const char *command);

static void clear_urg_communication_buffer(urg_t *urg, int timeout);
static int scip_response(urg_t *urg, const char* command,
                         const int expected_ret[], int timeout,
                         char *receive_buffer, int receive_buffer_max_size);
static void ignore_receive_data(urg_t *urg, int timeout);
static int change_sensor_baudrate(urg_t *urg,
                                  long current_baudrate, long next_baudrate);
static void ignore_receive_data_with_qt(urg_t *urg, int timeout);
static int set_errno_and_return(urg_t *urg, int urg_errno);
static const char *receive_command_response(urg_t *urg,
                                            char *buffer, int buffer_size,
                                            const char* command,
                                            int response_lines);
static char *copy_token(char *dest, char *receive_buffer,
                        const char *start_str, const char *end_ch, int lines);
static int safety_receive_data(urg_t *urg, long data[], unsigned short intensity[],
								safety_data_t *safety_data);
static int safety_receive_status_data(urg_t *urg, 
								status_data_t *status_data);

static int safety_receive_detection_log_data(urg_t *urg, 
								 detection_log_data_t *detection_log_data);

static int safety_receive_detection_log_clear(urg_t *urg);


const privateFuncPtr_t privateFunctions =
{
	clear_urg_communication_buffer,
	ignore_receive_data,
	change_sensor_baudrate,
	set_errno_and_return,
	safety_send_command,
	safety_response,
	safety_receive_data
};

//! \~japanese チェックサムの計算  \~english Calculates the checksum value
static char scip_checksum(const char buffer[], int size)
{
    unsigned char sum = 0x00;
    int i;

    for (i = 0; i < size; ++i) {
        sum += buffer[i];
    }

    // \~japanese 計算の意味は SCIP 仕様書を参照のこと
    // \~english Refer to the SCIP specification for details
    return (sum & 0x3f) + 0x30;
}


static int set_errno_and_return(urg_t *urg, int urg_errno)
{
    urg->last_errno = urg_errno;
    return urg_errno;
}


// \~japanese 受信した応答の行数を返す
// \~english Returns the number of lines received
static int scip_response(urg_t *urg, const char* command,
                         const int expected_ret[], int timeout,
                         char *receive_buffer, int receive_buffer_max_size)
{
    char *p = receive_buffer;
    char buffer[BUFFER_SIZE];
    int filled_size = 0;
    int line_number = 0;
    int ret = URG_UNKNOWN_ERROR;

    int write_size = (int)strlen(command);
    int n = connection_write(&urg->connection, command, write_size);

    if (n != write_size) {
        return set_errno_and_return(urg, URG_SEND_ERROR);
    }

    if (p) {
        *p = '\0';
    }

    do {
        n = connection_readline(&urg->connection, buffer, BUFFER_SIZE, timeout);
        if (n < 0) {
            return set_errno_and_return(urg, URG_NO_RESPONSE);

        } else if (p && (line_number > 0)
                   && (n < (receive_buffer_max_size - filled_size))) {
            // \~japanese エコーバックは完全一致のチェックを行うため、格納しない
	    // \~english Since echoback is verified upon completion, do not store it now
            memcpy(p, buffer, n);
            p += n;
            *p++ = '\0';
            filled_size += n;
        }

        if (line_number == 0) {
            // \~japanese エコーバック文字列が、一致するかを確認する
	    // \~english Check if the echoback is complete
            if (strncmp(buffer, command, write_size - 1)) {
                return set_errno_and_return(urg, URG_INVALID_RESPONSE);
            }
        } else if (n > 0 && !(line_number == 1 && n == 1)) {
            // \~japanese エコーバック以外の行のチェックサムを評価する(SCIP 1.1 応答の場合は無視する)
	    // \~english Checks the checksum value for all lines except the echoback (in SCIP1.1 this is ignored)
            char checksum = buffer[n - 1];
            if ((checksum != scip_checksum(buffer, n - 1)) &&
                (checksum != scip_checksum(buffer, n - 2))) {
                //return set_errno_and_return(urg, URG_CHECKSUM_ERROR); // <-- Due to a bug in VV, PP and II impelentation, checksum verification is disabled
            }
        }

        // \~japanese ステータス応答を評価して、戻り値を決定する
        // \~english Checks the status response to define the return value
        if (line_number == 1) {
            if (n == 1) {
                // \~japanese SCIP 1.1 応答の場合は、正常応答とみなす
				// \~english In case of SCIP1.1 it is always correct
                ret = 0;

            } else if (n != 3) {
                return set_errno_and_return(urg, URG_INVALID_RESPONSE);

            } else {
                int i;
                int actual_ret = strtol(buffer, NULL, 10);
                for (i = 0; expected_ret[i] != EXPECTED_END; ++i) {
                    if (expected_ret[i] == actual_ret) {
                        ret = 0;
                        break;
                    }
                }
            }
        }

        ++line_number;
    } while (n > 0);

    return (ret < 0) ? ret : (line_number - 1);
}

// \~japanese 受信した応答の行数を返す
// \~english Returns the number of lines received
static int safety_response(urg_t *urg, const char* command,
                         int timeout,
                         char *receive_buffer, int receive_buffer_max_size)
{
	enum {
		stx = 0x02,
        stx_size = 1,
		etx = 0x03,
		etx_size = 1,
        command_size = 4,
        header_size = 2,
        status_size = 2,
        crc_size = 4,
    };

    char *p = receive_buffer;
    char buffer[SAFETY_BUFFER_SIZE];
    int ret = URG_UNKNOWN_ERROR;
	unsigned short crc = 0;
	int n = 0;

    int write_size = (int)strlen(command);
	n = safety_send_command(urg, command);

    if (n < write_size) {
        return set_errno_and_return(urg, URG_SEND_ERROR);
    }

    if (p) {
        *p = '\0';
    }

    n = connection_readline(&urg->connection, buffer, SAFETY_BUFFER_SIZE, timeout);
    if (n < 0) {
        return set_errno_and_return(urg, URG_NO_RESPONSE);

    }else if(buffer[0] != stx){
		return set_errno_and_return(urg, URG_INVALID_RESPONSE);
	}

	if (p && (n < receive_buffer_max_size)) {
		memcpy(p, buffer + 1, n - 1);
		p += n - 1;
		*p++ = '\0';
	}

	if (n > 0) {
		if (safety_check_crc(buffer, n)) {
			return set_errno_and_return(urg, URG_CHECKSUM_ERROR);
		}
	}
	
    return n - 2;
}

static int safety_check_crc(const char* data, size_t length)
{
	enum {
        crc_size = 4,
    };

	unsigned short checksum = (unsigned short)strtol(&data[length - crc_size], NULL, 16);
	if (checksum != safety_calc_crc(data + 1, length - crc_size - 1)) {
		return -1;
	}

	return 0;
}

static void clear_urg_communication_buffer(urg_t *urg, int timeout)
{
    char buffer[BUFFER_SIZE];
    int n;

    connection_write(&urg->connection, "\n", 1);

    do {
        n = connection_readline(&urg->connection,
                                buffer, BUFFER_SIZE, timeout);
    } while (n >= 0);
}

static void ignore_receive_data(urg_t *urg, int timeout)
{
    char buffer[BUFFER_SIZE];
    int n;

    if (urg->is_sending == URG_FALSE) {
        return;
    }

    do {
        n = connection_readline(&urg->connection,
                                buffer, BUFFER_SIZE, timeout);
    } while (n >= 0);

    urg->is_sending = URG_FALSE;
}


static void ignore_receive_data_with_qt(urg_t *urg, int timeout)
{
    if ((urg->is_sending == URG_FALSE) && (urg->is_laser_on == URG_FALSE)) {
        return;
    }

    connection_write(&urg->connection, "QT\n", 3);
    urg->is_laser_on = URG_FALSE;
    ignore_receive_data(urg, timeout);
}


static int change_sensor_baudrate(urg_t *urg,
                                  long current_baudrate, long next_baudrate)
{
    enum { SS_COMMAND_SIZE = 10 };
    char buffer[SS_COMMAND_SIZE];
    int ss_expected[] = { 0, 3, 4, EXPECTED_END };
    int ret;

    if (current_baudrate == next_baudrate) {
        // \~japanese 現在のボーレートと設定するボーレートが一緒ならば、戻る
        // \~english If the current baudrate and next baudrate are the same, return
        return set_errno_and_return(urg, URG_NO_ERROR);
    }

    // \~japanese "SS" コマンドでボーレートを変更する
    // \~english Use "SS" command to change the baudrate
    snprintf(buffer, SS_COMMAND_SIZE, "SS%06ld\n", next_baudrate);
    ret = scip_response(urg, buffer, ss_expected, urg->timeout, NULL, 0);

    // \~japanese 0F 応答のときは Ethernet 用のセンサとみなし、正常応答を返す
    // \~english 0F is a correct response for Ethernet sensors
    if (ret == -15) {
        return set_errno_and_return(urg, URG_NO_ERROR);
    }
    if (ret <= 0) {
        return set_errno_and_return(urg, URG_INVALID_PARAMETER);
    }

    // \~japanese 正常応答ならば、ホスト側のボーレートを変更する
    // \~english If the result is correct, change the host's baudrate
    ret = connection_set_baudrate(&urg->connection, next_baudrate);

    ignore_receive_data(urg, MAX_TIMEOUT);

    // \~japanese センサ側の設定反映を待つために少しだけ待機する
    // \~english Waits a bit for the sensor to change baudrate
    urg_delay(150);

    return set_errno_and_return(urg, ret);
}


// \~japanese ボーレートを変更しながら接続する
// \~english Sets the baudrate and connects to the sensor
static int connect_urg_device(urg_t *urg, long baudrate)
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
        enum { RECEIVE_BUFFER_SIZE = 4 };
        int qt_expected[] = { 0, EXPECTED_END };
        char receive_buffer[RECEIVE_BUFFER_SIZE + 1];
        int ret;

        connection_set_baudrate(&urg->connection, try_baudrate[i]);

        // \~japanese URGが動いているボーレート以外でコマンドを送信した場合にゴミが残る場合があるのでクリア
        // \~english Clear URG read buffer to avoid having garbage data resulting from the incorrect baudrate communication
        clear_urg_communication_buffer(urg, MAX_TIMEOUT);

        // \~japanese QT を送信し、応答が返されるかでボーレートが一致しているかを確認する
	// \~english Sends the QT command and if the response is received then baudrate is correctly set
        ret = scip_response(urg, "QT\n", qt_expected, MAX_TIMEOUT,
                            receive_buffer, RECEIVE_BUFFER_SIZE);

        if (ret > 0) {
            if (!strcmp(receive_buffer, "E")) {
                int scip20_expected[] = { 0, EXPECTED_END };

                // \~japanese QT 応答の最後の改行を読み飛ばす
		// \~english Skips the end-of-line after the QT response
                ignore_receive_data(urg, MAX_TIMEOUT);

                // \~japanese "E" が返された場合は、SCIP 1.1 とみなし "SCIP2.0" を送信する
		// \~english If "E" is received it is SCIP1.1, so change to SCIP2.0
                ret = scip_response(urg, "SCIP2.0\n", scip20_expected,
                                    MAX_TIMEOUT, NULL, 0);

                // \~japanese SCIP2.0 応答の最後の改行を読み飛ばす
		// \~english Skips the end-of-line after SCIP2.0
                ignore_receive_data(urg, MAX_TIMEOUT);

                // \~japanese ボーレートを変更して戻る
		// \~english Changes the baudrate and returns
                return change_sensor_baudrate(urg, try_baudrate[i], baudrate);

            } else if (!strcmp(receive_buffer, "0Ee")) {
                int tm2_expected[] = { 0, EXPECTED_END };

                // \~japanese "0Ee" が返された場合は、TM モードとみなし "TM2" を送信する
		// \~english If "0Ee" is received it is in TM mode, send "TM2" to end it
                scip_response(urg, "TM2\n", tm2_expected,
                              MAX_TIMEOUT, NULL, 0);

                // \~japanese ボーレートを変更して戻る
		// \~english Changes the baudrate and returns
                return change_sensor_baudrate(urg, try_baudrate[i], baudrate);
            }
        }

        if (ret <= 0) {
            if (ret == URG_INVALID_RESPONSE) {
                // \~japanese 異常なエコーバックのときは、距離データ受信中とみなして
                // \~japanese データを読み飛ばす
	        // \~english If an invalid echoback is received, it is currently in measurement data transmission
	        // \~english so skip the data
                ignore_receive_data_with_qt(urg, MAX_TIMEOUT);

                // \~japanese ボーレートを変更して戻る
		// \~english Changes the baudrate and returns
                return change_sensor_baudrate(urg, try_baudrate[i], baudrate);

            } else {
                // \~japanese 応答がないときは、ボーレートを変更して、再度接続を行う
	        // \~english If there is no response, changes the baudrate and re-connects
                ignore_receive_data_with_qt(urg, MAX_TIMEOUT);
                continue;
            }
        } else if (!strcmp("00P", receive_buffer)) {

            // \~japanese センサとホストのボーレートを変更して戻る
	    // \~english Changes the baudrate and returns
            return change_sensor_baudrate(urg, try_baudrate[i], baudrate);
        }
    }

	safety_stop_measurement(urg);

    return set_errno_and_return(urg, URG_NOT_DETECT_BAUDRATE_ERROR);
}


// \~japanese PP コマンドの応答を urg_t に格納する
// \~english Stores the PP command response into urg_t
static int receive_parameter(urg_t *urg)
{
    enum { RECEIVE_BUFFER_SIZE = BUFFER_SIZE * 9, };
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    int pp_expected[] = { 0, EXPECTED_END };
    unsigned short received_bits = 0x0000;
    char *p;
    int i;

    int ret = scip_response(urg, "PP\n", pp_expected, MAX_TIMEOUT,
                            receive_buffer, RECEIVE_BUFFER_SIZE);
    if (ret < 0) {
        return ret;
    } else if (ret < PP_RESPONSE_LINES) {
        ignore_receive_data_with_qt(urg, MAX_TIMEOUT);
        return set_errno_and_return(urg, URG_INVALID_RESPONSE);
    }

    p = receive_buffer;
    for (i = 0; i < (ret - 1); ++i) {

        if (!strncmp(p, "DMIN:", 5)) {
            urg->min_distance = strtol(p + 5, NULL, 10);
            received_bits |= 0x0001;

        } else if (!strncmp(p, "DMAX:", 5)) {
            urg->max_distance = strtol(p + 5, NULL, 10);
            received_bits |= 0x0002;

        } else if (!strncmp(p, "ARES:", 5)) {
            urg->area_resolution = strtol(p + 5, NULL, 10);
            received_bits |= 0x0004;

        } else if (!strncmp(p, "AMIN:", 5)) {
            urg->first_data_index = strtol(p + 5, NULL, 10);
            received_bits |= 0x0008;

        } else if (!strncmp(p, "AMAX:", 5)) {
            urg->last_data_index = strtol(p + 5, NULL, 10);
            received_bits |= 0x0010;

        } else if (!strncmp(p, "AFRT:", 5)) {
            urg->front_data_index = strtol(p + 5, NULL, 10);
            received_bits |= 0x0020;

        } else if (!strncmp(p, "SCAN:", 5)) {
            int rpm = strtol(p + 5, NULL, 10);
            // \~japanese タイムアウト時間は、計測周期の 16 倍程度の値にする
	    // \~english Timeout is set about 16 times the sensor measurement period
            urg->scan_usec = 1000 * 1000 * 60 / rpm;
            urg->timeout = urg->scan_usec >> (10 - 4);
            received_bits |= 0x0040;
        }
        p += strlen(p) + 1;
    }

    // \~japanese 全てのパラメータを受信したか確認
    // \~english Check if all the parameters were received
    if (received_bits != 0x007f) {
        return set_errno_and_return(urg, URG_RECEIVE_ERROR);
    }

    urg_set_scanning_parameter(urg,
                               urg->first_data_index - urg->front_data_index,
                               urg->last_data_index - urg->front_data_index,
                               1);

    return set_errno_and_return(urg, URG_NO_ERROR);
}


//! \~japanese SCIP 文字列のデコード  \~english  Decodes the SCIP message
long urg_scip_decode(const char data[], int size)
{
    const char* p = data;
    const char* last_p = p + size;
    int value = 0;

    while (p < last_p) {
        value <<= 6;
        value &= ~0x3f;
        value |= *p++ - 0x30;
    }
    return value;
}


static int parse_parameter(const char *parameter, int size)
{
    char buffer[5];

    memcpy(buffer, parameter, size);
    buffer[size] = '\0';

    return strtol(buffer, NULL, 10);
}


static urg_measurement_type_t parse_distance_parameter(urg_t *urg,
                                                       const char echoback[])
{
    urg_measurement_type_t ret_type = URG_UNKNOWN;

    urg->received_range_data_byte = URG_COMMUNICATION_3_BYTE;
    if (echoback[1] == 'S') {
        urg->received_range_data_byte = URG_COMMUNICATION_2_BYTE;
        ret_type = URG_DISTANCE;

    } else if (echoback[1] == 'D') {
        if ((echoback[0] == 'G') || (echoback[0] == 'M')) {
            ret_type = URG_DISTANCE;
        } else if ((echoback[0] == 'H') || (echoback[0] == 'N')) {
            ret_type = URG_MULTIECHO;
        }
    } else if (echoback[1] == 'E') {
        if ((echoback[0] == 'G') || (echoback[0] == 'M')) {
            ret_type = URG_DISTANCE_INTENSITY;
        } else if ((echoback[0] == 'H') || (echoback[0] == 'N')) {
            ret_type = URG_MULTIECHO_INTENSITY;
        }
    } else {
        return URG_UNKNOWN;
    }

    // \~japanese パラメータの格納
    // \~english Stores the parameters
    urg->received_first_index = parse_parameter(&echoback[2], 4);
    urg->received_last_index = parse_parameter(&echoback[6], 4);
    urg->received_skip_step = parse_parameter(&echoback[10], 2);

    return ret_type;
}


static urg_measurement_type_t parse_distance_echoback(urg_t *urg,
                                                      const char echoback[])
{
    size_t line_length;
    urg_measurement_type_t ret_type = URG_UNKNOWN;

    if (!strcmp("QT", echoback)) {
        return URG_STOP;
    }

    line_length = strlen(echoback);
    if ((line_length == 12) &&
        ((echoback[0] == 'G') || (echoback[0] == 'H'))) {
        ret_type = parse_distance_parameter(urg, echoback);

    } else if ((line_length == 15) &&
               ((echoback[0] == 'M') || (echoback[0] == 'N'))) {
        ret_type = parse_distance_parameter(urg, echoback);
    }
    return ret_type;
}


static int receive_length_data(urg_t *urg, long length[],
                               unsigned short intensity[],
                               urg_measurement_type_t type, char buffer[])
{
    int n;
    int step_filled = 0;
    int line_filled = 0;
    int multiecho_index = 0;

    int each_size =
        (urg->received_range_data_byte == URG_COMMUNICATION_2_BYTE) ? 2 : 3;
    int data_size = each_size;
    int is_intensity = URG_FALSE;
    int is_multiecho = URG_FALSE;
    int multiecho_max_size = 1;

    if ((type == URG_DISTANCE_INTENSITY) || (type == URG_MULTIECHO_INTENSITY)) {
        data_size *= 2;
        is_intensity = URG_TRUE;
    }
    if ((type == URG_MULTIECHO) || (type == URG_MULTIECHO_INTENSITY)) {
        is_multiecho = URG_TRUE;
        multiecho_max_size = URG_MAX_ECHO;
    }

    do {
        char *p = buffer;
        char *last_p;

        n = connection_readline(&urg->connection,
                                &buffer[line_filled], BUFFER_SIZE - line_filled,
                                urg->timeout);

        if (n > 0) {
            // \~japanese チェックサムの評価
	    // \~english Validates the checksum
            if (buffer[line_filled + n - 1] !=
                scip_checksum(&buffer[line_filled], n - 1)) {
                ignore_receive_data_with_qt(urg, urg->timeout);
                return set_errno_and_return(urg, URG_CHECKSUM_ERROR);
            }
        }

        if (n > 0) {
            line_filled += n - 1;
        }
        last_p = p + line_filled;

        while ((last_p - p) >= data_size) {
            int index;

            if (*p == '&') {
                // \~japanese 先頭文字が '&' だったときは、マルチエコーのデータとみなす
                // \~english If the start character is a '&' then assume data is multiecho
                if ((last_p - (p + 1)) < data_size) {
                    // \~japanese '&' を除いて、data_size 分データが無ければ抜ける
		    // \~english Skips the '&' and if the string size is less than data_size ignore it
                    break;
                }

                --step_filled;
                ++multiecho_index;
                ++p;
                --line_filled;

            } else {
                // \~japanese 次のデータ
	        // \~english Next data
                multiecho_index = 0;
            }

            index = (step_filled * multiecho_max_size) + multiecho_index;

            if (step_filled >
                (urg->received_last_index - urg->received_first_index)) {
                // \~japanese データが多過ぎる場合は、残りのデータを無視して戻る
	        // \~english If there is extra data, ignore it
                ignore_receive_data_with_qt(urg, urg->timeout);
                return set_errno_and_return(urg, URG_RECEIVE_ERROR);
            }


            if (is_multiecho && (multiecho_index == 0)) {
                // \~japanese マルチエコーのデータ格納先をダミーデータで埋める
	        // \~english Stores dummy values in the multiecho data location
                int i;
                if (length) {
                    for (i = 1; i < multiecho_max_size; ++i) {
                        length[index + i] = 0;
                    }
                }
                if (intensity) {
                    for (i = 1; i < multiecho_max_size; ++i) {
                        intensity[index + i] = 0;
                    }
                }
            }

            // \~japanese 距離データの格納
            // \~english Stores the distance data
            if (length) {
                length[index] = urg_scip_decode(p, 3);
            }
            p += 3;

            // \~japanese 強度データの格納
	    // \~english Stores the intensity data
            if (is_intensity) {
                if (intensity) {
                    intensity[index] = (unsigned short)urg_scip_decode(p, 3);
                }
                p += 3;
            }

            ++step_filled;
            line_filled -= data_size;
        }

        // \~japanese 次に処理する文字を退避
        // \~english Prepares the next line to process
        memmove(buffer, p, line_filled);
    } while (n > 0);

    return step_filled;
}


//! \~japanese 距離データの取得  \~english Gets measurement data
static int receive_data(urg_t *urg, long data[], unsigned short intensity[],
                        long *time_stamp)
{
    urg_measurement_type_t type;
    char buffer[BUFFER_SIZE];
    int ret = 0;
    int n;
    int extended_timeout = urg->timeout
        + 2 * (urg->scan_usec * (urg->scanning_skip_scan) / 1000);

    // \~japanese エコーバックの取得
    // \~english Gets the echoback
    n = connection_readline(&urg->connection,
                            buffer, BUFFER_SIZE, extended_timeout);
    if (n <= 0) {
        return set_errno_and_return(urg, URG_NO_RESPONSE);
    }
    // \~japanese エコーバックの解析
    // \~english Checks the echoback
    type = parse_distance_echoback(urg, buffer);

    // \~japanese 応答の取得
    // \~english Gets the response message
    n = connection_readline(&urg->connection,
                            buffer, BUFFER_SIZE, urg->timeout);
    if (n != 3) {
        ignore_receive_data_with_qt(urg, urg->timeout);
        return set_errno_and_return(urg, URG_INVALID_RESPONSE);
    }

    if (buffer[n - 1] != scip_checksum(buffer, n - 1)) {
        // \~japanese チェックサムの評価
        // \~english Validates the checksum
        ignore_receive_data_with_qt(urg, urg->timeout);
        return set_errno_and_return(urg, URG_CHECKSUM_ERROR);
    }

    if (type == URG_STOP) {
        // \~japanese QT 応答の場合には、最後の改行を読み捨て、正常応答として処理する
        // \~english If received QT response, ignore the last end-of-line and return as successful
        n = connection_readline(&urg->connection,
                                buffer, BUFFER_SIZE, urg->timeout);
        if (n == 0) {
            return 0;
        } else {
            return set_errno_and_return(urg, URG_INVALID_RESPONSE);
        }
    }

    if (urg->specified_scan_times != 1) {
        if (!strncmp(buffer, "00", 2)) {
            // \~japanese "00" 応答の場合は、エコーバック応答とみなし、
            // \~japanese 最後の空行を読み捨て、次からのデータを返す
	    // \~english If received "00" response, assumes it is the echoback
	    // \~english Skips up to the next empty line and returns the next data
            n = connection_readline(&urg->connection,
                                    buffer, BUFFER_SIZE, urg->timeout);

            if (n != 0) {
                ignore_receive_data_with_qt(urg, urg->timeout);
                return set_errno_and_return(urg, URG_INVALID_RESPONSE);
            } else {
                return receive_data(urg, data, intensity, time_stamp);
            }
        }
    }

    if (((urg->specified_scan_times == 1) && (strncmp(buffer, "00", 2))) ||
        ((urg->specified_scan_times != 1) && (strncmp(buffer, "99", 2)))) {
        if (urg->error_handler) {
            type = urg->error_handler(buffer, urg);
        }

        //if (type == URG_UNKNOWN) {
            // \~japanese Gx, Hx のときは 00P が返されたときがデータ
            // \~japanese Mx, Nx のときは 99b が返されたときがデータ
            // \~english If received "00P" response for Gx/Hx
            // \~english If received "99b" response for Mx/Nx
            ignore_receive_data_with_qt(urg, urg->timeout);
            return set_errno_and_return(urg, URG_INVALID_RESPONSE);
        //}
    }

    // \~japanese タイムスタンプの取得
    // \~english Gets the timestamp
    n = connection_readline(&urg->connection,
                            buffer, BUFFER_SIZE, urg->timeout);
    if (n > 0) {
        if (time_stamp) {
            *time_stamp = urg_scip_decode(buffer, 4);
        }
    }

    // \~japanese データの取得
    // \~english Gets the measurement data
    switch (type) {
    case URG_DISTANCE:
    case URG_MULTIECHO:
        ret = receive_length_data(urg, data, NULL, type, buffer);
        break;

    case URG_DISTANCE_INTENSITY:
    case URG_MULTIECHO_INTENSITY:
        ret = receive_length_data(urg, data, intensity, type, buffer);
        break;

    case URG_STOP:
    case URG_UNKNOWN:
        ret = 0;
        break;
    }

    // \~japanese specified_scan_times == 1 のときは Gx 系コマンドが使われるため
    // \~japanese データを明示的に停止しなくてよい
    // \~english If specified_scan_times == 1 then we are using a Gx type command
    // \~english it is not necessary to explicity stop measurement
    if ((urg->specified_scan_times > 1) && (urg->scanning_remain_times > 0)) {
        if (--urg->scanning_remain_times <= 0) {
            // \~japanese データの停止のみを行う
	    // \~english Stops measurement
            urg_stop_measurement(urg);
        }
    }
    return ret;
}

//! \~japanese 距離データの取得  \~english Gets measurement data
static int safety_receive_data(urg_t *urg, long data[], unsigned short intensity[],
								safety_data_t *safety_data)
{
	enum {
		STX = 0x02,
		ETX = 0x03,
        STX_SIZE = 1,
		ETX_SIZE = 1,
        LENGTH_SIZE = 4,
		COMMAND_SIZE = 2,
        SUB_COMMAND_SIZE = 2,
        STATUS_SIZE = 2,
        CRC_SIZE = 4,
        COMMAND_OFFSET = STX_SIZE + LENGTH_SIZE,
        SUB_COMMAND_OFFSET = COMMAND_OFFSET + COMMAND_SIZE,
        STATUS_OFFSET = SUB_COMMAND_OFFSET + COMMAND_SIZE,
		DATA_OFFSET = STATUS_OFFSET + STATUS_SIZE,
		RESPONSE_MIN_LENGTH = DATA_OFFSET + CRC_SIZE,     //excluding ETX size
    };
    urg_measurement_type_t type = URG_UNKNOWN;
    char buffer[SAFETY_BUFFER_SIZE];
	char param[9];
	char *data_buffer = 0;
    int ret = 0;
    int n, i, cp;
    int extended_timeout = urg->timeout
        + 2 * (urg->scan_usec * (urg->scanning_skip_scan) / 1000);
	int first_index = urg->first_data_index;
	int last_index = urg->last_data_index;
    int status = 0;

    // \~japanese エコーバックの取得
    // \~english Gets the echoback
    n = connection_readline(&urg->connection,
                            buffer, SAFETY_BUFFER_SIZE, extended_timeout * 10);
    if (n <= 0) {
        return set_errno_and_return(urg, URG_NO_RESPONSE);
    }

	buffer[n] = '\0';

	if ((n < RESPONSE_MIN_LENGTH) || (buffer[0] != STX)) {
        return set_errno_and_return(urg, URG_INVALID_RESPONSE);
    }

    // \~japanese エコーバックの解析
    // \~english Checks the echoback
    if ((buffer[COMMAND_OFFSET] == 'A') && (buffer[COMMAND_OFFSET + 1] == 'R')) {
        if ((buffer[SUB_COMMAND_OFFSET + 1] == '0') || (buffer[SUB_COMMAND_OFFSET + 1] == '2')) {
            type = URG_DISTANCE;
        }
        else if ((buffer[SUB_COMMAND_OFFSET + 1] == '1') || (buffer[SUB_COMMAND_OFFSET + 1] == '4')) {
            type = URG_DISTANCE_INTENSITY;
        }
        else if ((buffer[SUB_COMMAND_OFFSET + 1] == '6') || (buffer[SUB_COMMAND_OFFSET + 1] == '7')) {
            type = URG_DISTANCE;
            last_index = urg->last_data_index_fullstep;
        }
        else if ((buffer[SUB_COMMAND_OFFSET + 1] == '3') || (buffer[SUB_COMMAND_OFFSET + 1] == '5') || (buffer[SUB_COMMAND_OFFSET + 1] == '8')) {
            type = URG_STOP;
        }
        else {
            return set_errno_and_return(urg, URG_INVALID_RESPONSE);
        }
    }
    else {
        return set_errno_and_return(urg, URG_INVALID_RESPONSE);
    }

    //Check CRC
	if (safety_check_crc(buffer, n)) {
        return set_errno_and_return(urg, URG_CHECKSUM_ERROR);
    }

    // ETXが読み捨てられているため、電文長は1短い
    if (n == RESPONSE_MIN_LENGTH) {
        if (type == URG_STOP) {
            return 0;
        }
        else {
            //check the status
            strncpy(param, &buffer[STATUS_OFFSET], 2);
            param[2] = '\0';
            status = strtol(param, NULL, 16);
            if (status > 0) {
                return set_errno_and_return(urg, status);
            }
            return 0;
        }
    }

	if (safety_data)
	{
		safety_data->is_setting = buffer[DATA_OFFSET] == '1' ? 1 : 0;

		strncpy(param, &buffer[DATA_OFFSET +1], 2);
		param[2] = '\0';
		safety_data->area_number = strtol(param, NULL, 16);

		safety_data->is_error_detected = buffer[DATA_OFFSET + 3] == '1' ? 1 : 0;

		strncpy(param, &buffer[DATA_OFFSET +4], 2);
		param[2] = '\0';
		safety_data->error_code = strtol(param, NULL, 16);

		safety_data->is_lockout = buffer[DATA_OFFSET + 6] == '1' ? 1 : 0;

		safety_data->is_ossd1_on = buffer[DATA_OFFSET + 7] == '1' ? 1 : 0;
		safety_data->is_ossd2_on = buffer[DATA_OFFSET + 8] == '1' ? 1 : 0;

		safety_data->is_warning1_on = buffer[DATA_OFFSET + 9] == '1' ? 1 : 0;
		safety_data->is_warning2_on = buffer[DATA_OFFSET + 10] == '1' ? 1 : 0;

		safety_data->is_ossd3_on = buffer[DATA_OFFSET + 11] == '1' ? 1 : 0;
		safety_data->is_ossd4_on = buffer[DATA_OFFSET + 12] == '1' ? 1 : 0;

		safety_data->reserved1 = buffer[DATA_OFFSET + 13] == '1' ? 1 : 0;
		safety_data->reserved2 = buffer[DATA_OFFSET + 14] == '1' ? 1 : 0;
		
		safety_data->is_mut_over1_on = buffer[DATA_OFFSET + 15] == '1' ? 1 : 0;
		safety_data->is_mut_over2_on = buffer[DATA_OFFSET + 16] == '1' ? 1 : 0;

		safety_data->is_reset_req1_on = buffer[DATA_OFFSET + 17] == '1' ? 1 : 0;
		safety_data->is_reset_req2_on = buffer[DATA_OFFSET + 18] == '1' ? 1 : 0;

		strncpy(param, &buffer[DATA_OFFSET +19], 4);
		param[4] = '\0';
		safety_data->encoder_velocity = strtol(param, NULL, 16);

		strncpy(param, &buffer[DATA_OFFSET +23], 8);
		param[8] = '\0';
		safety_data->timestamp = strtol(param, NULL, 16);

		safety_data->is_laser_off =  buffer[DATA_OFFSET + 31] == '1' ? 1 : 0;
		safety_data->is_optical_window_contamination_warning =  buffer[DATA_OFFSET + 32] == '1' ? 1 : 0;
	    
        strncpy(param, &buffer[DATA_OFFSET + 33], 1);
        param[1] = '\0';
        safety_data->encoder_input_pattern_num = strtol(param, NULL, 16);

        strncpy(param, &buffer[DATA_OFFSET + 34], 4);
        param[4] = '\0';
        safety_data->encoder_angular_velocity = strtol(param, NULL, 16);

        strncpy(param, &buffer[DATA_OFFSET + 39], 4);
        param[4] = '\0';
        safety_data->protection_z1_detect_start_step = strtol(param, NULL, 16);

        strncpy(param, &buffer[DATA_OFFSET + 43], 4);
        param[4] = '\0';
        safety_data->protection_z1_detect_end_step = strtol(param, NULL, 16);

        strncpy(param, &buffer[DATA_OFFSET + 47], 4);
        param[4] = '\0';
        safety_data->protection_z2_detect_start_step = strtol(param, NULL, 16);

        strncpy(param, &buffer[DATA_OFFSET + 51], 4);
        param[4] = '\0';
        safety_data->protection_z2_detect_end_step = strtol(param, NULL, 16);

        strncpy(param, &buffer[DATA_OFFSET + 55], 4);
        param[4] = '\0';
        safety_data->warning_z1_detect_start_step = strtol(param, NULL, 16);

        strncpy(param, &buffer[DATA_OFFSET + 59], 4);
        param[4] = '\0';
        safety_data->warning_z1_detect_end_step = strtol(param, NULL, 16);

        strncpy(param, &buffer[DATA_OFFSET + 63], 4);
        param[4] = '\0';
        safety_data->warning_z2_detect_start_step = strtol(param, NULL, 16);

        strncpy(param, &buffer[DATA_OFFSET + 67], 4);
        param[4] = '\0';
        safety_data->warning_z2_detect_end_step = strtol(param, NULL, 16);
    }

	data_buffer = &buffer[DATA_OFFSET + 71];

    // \~japanese データの取得
    // \~english Gets the measurement data
    switch (type) {
    case URG_DISTANCE:
    case URG_MULTIECHO:
		cp = 0;
		for(i = first_index; i <= last_index; i++, cp++){
			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			if (data)
			{
				data[cp] = strtol(param, NULL, 16);
			}
			data_buffer += 4;
		}
		ret = cp;
        break;

    case URG_DISTANCE_INTENSITY:
    case URG_MULTIECHO_INTENSITY:
		cp = 0;
		for(i = first_index; i <= last_index; i++, cp++){
			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			if (data)
			{
				data[cp] = strtol(param, NULL, 16);
			}
			data_buffer += 4;
		}
		if(intensity){
			cp = 0;
			for(i = first_index; i <= last_index; i++, cp++){
				strncpy(param, data_buffer, 4);
				param[4] = '\0';
				intensity[cp] = (unsigned short)strtol(param, NULL, 16);
				data_buffer += 4;
			}
		}
		ret = cp;
        break;

    case URG_STOP:
    case URG_UNKNOWN:
        ret = 0;
        break;
    }

	urg->received_first_index = urg->first_data_index;
	urg->received_last_index = urg->last_data_index; 
    urg->received_skip_step = 0;
    return ret;
}

//! \~japanese 距離データの取得  \~english Gets measurement data
static int safety_receive_area_data(urg_t *urg, long data[])
{
	enum {
		stx = 0x02,
        stx_size = 1,
		etx = 0x03,
		etx_size = 1,
        command_size = 4,
		body_size = 4,
        crc_size = 4,
		body_offset = 1 + 4 + 4 + 2 + 4 + 4 + 2 + 2,
		YR_STOP_RETRUN_LENGTH = 28
    };
    urg_measurement_type_t type = URG_UNKNOWN;
    char buffer[SAFETY_BUFFER_SIZE];
	char param[9];
	char *data_buffer = 0;
    int ret = 0;
    int n, i, cp;
    int extended_timeout = urg->timeout
        + 2 * (urg->scan_usec * (urg->scanning_skip_scan) / 1000);
	int first_index = urg->first_data_index;
	int last_index = urg->last_data_index;

    // \~japanese エコーバックの取得
    // \~english Gets the echoback
    n = connection_readline(&urg->connection,
                            buffer, SAFETY_BUFFER_SIZE, extended_timeout * 10);
    if (n <= 0) {
        return set_errno_and_return(urg, URG_NO_RESPONSE);
    }

	buffer[n] = '\0';

	if ((n < body_size) || (buffer[0] != stx)) {
        return set_errno_and_return(urg, URG_INVALID_RESPONSE);
    }

   	if (safety_check_crc(buffer, n)) {
        return set_errno_and_return(urg, URG_CHECKSUM_ERROR);
    }

    if (type == URG_STOP) {
		// ETXが読み捨てられているため、電文長は1短い
		if (n ==YR_STOP_RETRUN_LENGTH - 1) {
            return 0;
        } else {
            return set_errno_and_return(urg, URG_INVALID_RESPONSE);
        }
    }

	if(n <= body_offset + crc_size){
		return 0;
	}

	
	data_buffer = &buffer[body_offset];

    // \~japanese データの取得
    // \~english Gets the area data
   
	cp = 0;
	for(i = first_index; i <= last_index; i++, cp++){
		strncpy(param, data_buffer, 4);
		param[4] = '\0';
		if (data)
		{
			data[cp] = strtol(param, NULL, 16);
		}
		data_buffer += 4;
	}
	ret = cp;
       

	urg->received_first_index = urg->first_data_index;
	urg->received_last_index = urg->last_data_index;
    urg->received_skip_step = 0;
    return ret;
}

int urg_open(urg_t *urg, urg_connection_type_t connection_type,
             const char *device_or_address, long baudrate_or_port)
{
    int ret;
    long baudrate = baudrate_or_port;

    urg->is_active = URG_FALSE;
    urg->is_sending = URG_TRUE;
    urg->last_errno = URG_NOT_CONNECTED;
    urg->timeout = MAX_TIMEOUT;
    urg->scanning_skip_scan = 0;
    urg->error_handler = NULL;

    // \~japanese デバイスへの接続
    // \~english Connects to the device
    ret = connection_open(&urg->connection, connection_type,
                          device_or_address, baudrate_or_port);

    if (ret < 0) {
        switch (connection_type) {
        case URG_SERIAL:
            urg->last_errno = URG_SERIAL_OPEN_ERROR;
            break;

        case URG_ETHERNET:
            urg->last_errno = URG_ETHERNET_OPEN_ERROR;
            break;

        default:
            urg->last_errno = URG_INVALID_RESPONSE;
            break;
        }
        return urg->last_errno;
    }

    // \~japanese  指定したボーレートで URG と通信できるように調整
    // \~english Make adjustments so to connect with URG using the specified baudrate
    if (connection_type == URG_ETHERNET) {
        // \~japanese  Ethernet のときは仮の通信速度を指定しておく
        // \~english In case of Ethernet, sets a fake baudrate
        baudrate = 115200;
    }

    ret = connect_urg_device(urg, baudrate);
    if (ret != URG_NO_ERROR) {
        return set_errno_and_return(urg, ret);
    }
    urg->is_sending = URG_FALSE;

    // \~japanese  変数の初期化
    // \~english Initializes variables
    urg->last_errno = URG_NO_ERROR;
    urg->range_data_byte = URG_COMMUNICATION_3_BYTE;
    urg->specified_scan_times = 0;
    urg->scanning_remain_times = 0;
    urg->is_laser_on = URG_FALSE;

    // \~japanese  パラメータ情報を取得
    // \~english Gets the sensor parameters
    ret = receive_parameter(urg);
    if (ret == URG_NO_ERROR) {
        urg->is_active = URG_TRUE;
    }

	safety_init_crc();
    return ret;
}


void urg_close(urg_t *urg)
{
    if (urg->is_active) {
        ignore_receive_data_with_qt(urg, urg->timeout);
    }
    connection_close(&urg->connection);
    urg->is_active = URG_FALSE;
}


void urg_set_timeout_msec(urg_t *urg, int msec)
{
    urg->timeout = msec;
}


int urg_start_time_stamp_mode(urg_t *urg)
{
    const int expected[] = { 0, EXPECTED_END };
    int n;

    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    // \~japanese  TM0 を発行する
    // \~english Sends TM0 command
    n = scip_response(urg, "TM0\n", expected, urg->timeout, NULL, 0);
    if (n <= 0) {
        return set_errno_and_return(urg, URG_INVALID_RESPONSE);
    } else {
        return 0;
    }
}


long urg_time_stamp(urg_t *urg)
{
    const int expected[] = { 0, EXPECTED_END };
    char buffer[BUFFER_SIZE];
    char *p;
    int ret;

    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    ret = scip_response(urg, "TM1\n", expected,
                        urg->timeout, buffer, BUFFER_SIZE);
    if (ret < 0) {
        return ret;
    }

    // \~japanese  buffer からタイムスタンプを取得し、デコードして返す
    // \~english Gets and decodes the timestamp stored in buffer
    if (strcmp(buffer, "00P")) {
        // \~japanese  最初の応答が "00P" でなければ戻る
        // \~english If first status code is not "00P" return
        return set_errno_and_return(urg, URG_RECEIVE_ERROR);
    }
    p = buffer + 4;
    if (strlen(p) != 5) {
        return set_errno_and_return(urg, URG_RECEIVE_ERROR);
    }
    if (p[5] == scip_checksum(p, 4)) {
        return set_errno_and_return(urg, URG_CHECKSUM_ERROR);
    }
    return urg_scip_decode(p, 4);
}


int urg_stop_time_stamp_mode(urg_t *urg)
{
    int expected[] = { 0, EXPECTED_END };
    int n;

    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    // \~japanese  TM2 を発行する
    // \~english Sends the TM2 command
    n = scip_response(urg, "TM2\n", expected, urg->timeout, NULL, 0);
    if (n <= 0) {
        return set_errno_and_return(urg, URG_INVALID_RESPONSE);
    } else {
        return 0;
    }
}


static int send_distance_command(urg_t *urg, int scan_times, int skip_scan,
                                 char single_scan_ch, char continuous_scan_ch,
                                 char scan_type_ch)
{
    char buffer[BUFFER_SIZE];
    int write_size = 0;
    int front_index = urg->front_data_index;
    int n;

    urg->specified_scan_times = (scan_times < 0) ? 0 : scan_times;
    urg->scanning_remain_times = urg->specified_scan_times;
    urg->scanning_skip_scan = (skip_scan < 0) ? 0 : skip_scan;
    if (scan_times >= 100) {
        // \~japanese  計測回数が 99 を越える場合は、無限回のスキャンを行う
        // \~english If the number of scans is over 99, work in infinite scanning mode
        urg->specified_scan_times = 0;
    }

    //if (urg->scanning_remain_times == 1) {
    //    // \~japanese  レーザ発光を指示
    //    // \~english Prepares the measurement command
    //    urg_laser_on(urg);

    //    write_size = snprintf(buffer, BUFFER_SIZE, "%c%c%04d%04d%02d\n",
    //                          single_scan_ch, scan_type_ch,
    //                          urg->scanning_first_step + front_index,
    //                          urg->scanning_last_step + front_index,
    //                          urg->scanning_skip_step);
    //} else {
        write_size = snprintf(buffer, BUFFER_SIZE, "%c%c%04d%04d%02d%01d%02d\n",
                              continuous_scan_ch, scan_type_ch,
                              urg->scanning_first_step + front_index,
                              urg->scanning_last_step + front_index,
                              urg->scanning_skip_step,
                              skip_scan, urg->specified_scan_times);
        urg->is_sending = URG_TRUE;
    //}

    n = connection_write(&urg->connection, buffer, write_size);
    if (n != write_size) {
        return set_errno_and_return(urg, URG_SEND_ERROR);
    }

    return 0;
}


int urg_start_measurement(urg_t *urg, urg_measurement_type_t type,
                          int scan_times, int skip_scan)
{
    char range_byte_ch;
    int ret = 0;

    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    if ((skip_scan < 0) || (skip_scan > 9)) {
        ignore_receive_data_with_qt(urg, urg->timeout);
        return set_errno_and_return(urg, URG_INVALID_PARAMETER);
    }

    // \~japanese  !!! Mx 系, Nx 系の計測中のときは、QT を発行してから
    // \~japanese  !!! 計測開始コマンドを送信するようにする
    // \~japanese  !!! ただし、MD 計測中に MD を発行するように、同じコマンドの場合は
    // \~japanese  !!! Mx 系, Nx 系の計測は上書きすることができるようにする
    // \~english   !!! If a Mx/Nx measurement command is already in operation,
    // \~english   !!! measurement can be re-started after having send QT command.
    // \~english   !!! However, if Mx/Nx is already running and the same command is issued again
    // \~english   !!! (ex., send MD command while another MD is running), the command is overwritten

    // \~japanese  指定されたタイプのパケットを生成し、送信する
    // \~english Prepares and sends the measurement command according to the given type
    switch (type) {
    case URG_DISTANCE:
        range_byte_ch =
            (urg->range_data_byte == URG_COMMUNICATION_2_BYTE) ? 'S' : 'D';
        ret = send_distance_command(urg, scan_times, skip_scan,
                                    'G', 'M', range_byte_ch);
        break;

    case URG_DISTANCE_INTENSITY:
        ret = send_distance_command(urg, scan_times, skip_scan,
                                    'G', 'M', 'E');
        break;

    case URG_MULTIECHO:
        ret = send_distance_command(urg, scan_times, skip_scan,
                                    'H', 'N', 'D');
        break;

    case URG_MULTIECHO_INTENSITY:
        ret = send_distance_command(urg, scan_times, skip_scan,
                                    'H', 'N', 'E');
        break;

    case URG_STOP:
    case URG_UNKNOWN:
    default:
        ignore_receive_data_with_qt(urg, urg->timeout);
        urg->last_errno = URG_INVALID_PARAMETER;
        ret = urg->last_errno;
        break;
    }

    return ret;
}

static int safety_send_command(urg_t *urg, const char *command)
{
	enum {
		stx = 0x02,
        stx_size = 1,
		etx = 0x03,
		etx_size = 1,
        length_size = 4,
        crc_size = 4,
    };
	char buffer[SAFETY_BUFFER_SIZE];
    int ret = 0;
	unsigned short crc = 0;
	int message_size = 0;
	int length = strlen(command);
	
	message_size = stx_size + length_size + length + crc_size + etx_size;

	snprintf(buffer, SAFETY_BUFFER_SIZE, "%0*X%s", length_size, message_size, command);
	crc = safety_calc_crc(buffer, length_size + length);
	snprintf(buffer, SAFETY_BUFFER_SIZE, "%c%0*X%s%0*X%c", stx, length_size, message_size, command, crc_size, crc, etx);
    ret = connection_write(&urg->connection, buffer, message_size);
}

int safety_start_measurement(urg_t *urg, urg_measurement_type_t type, urg_measurement_mode_t mode)
{
	enum {
		stx = 0x02,
        stx_size = 1,
		etx = 0x03,
		etx_size = 1,
        command_size = 4,
		body_size = 4,
        crc_size = 4,
    };
    char command [body_size + 1];
    int ret = 0;
	unsigned short crc = 0;
	int message_size = 0;

    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    // \~japanese  指定されたタイプのパケットを生成し、送信する
    // \~english Prepares and sends the measurement command according to the given type
    switch (type) {
    case URG_DISTANCE:
        switch (mode){
		case URG_HANDSHAKE:
			snprintf(command, body_size, "AR00");
			break;
		case URG_CONTINUOUS:
			snprintf(command, body_size, "AR02");
			break;
		}
        break;

    case URG_DISTANCE_INTENSITY:
        switch (mode){
		case URG_HANDSHAKE:
			snprintf(command, body_size, "AR01");
			break;
		case URG_CONTINUOUS:
			snprintf(command, body_size, "AR04");
			break;
		}
        break;

    case URG_STOP:
    case URG_UNKNOWN:
    default:
        switch (mode){
		case URG_CONTINUOUS:
			snprintf(command, body_size, "AR03");
			break;
		}
        break;
    }
	command[body_size] = '\0'; 

	return safety_send_command(urg, command);
}

//! \~japanese ステータスデータの取得  \~english Gets status data
static int safety_receive_status_data(urg_t *urg, 
								status_data_t *status_data)
{
	enum {
		stx = 0x02,
        stx_size = 1,
		etx = 0x03,
		etx_size = 1,
        command_size = 4,
		body_size = 4,
        crc_size = 4,
		body_offset = 1 + 4 + 4 + 2,
    };

    char buffer[SAFETY_BUFFER_SIZE];
	char param[9];
	char *data_buffer = 0;
    int ret = 0;
    int n, i;
    int extended_timeout = urg->timeout
        + 2 * (urg->scan_usec * (urg->scanning_skip_scan) / 1000);
	int first_index = urg->first_data_index;
	int last_index = urg->last_data_index;

    // \~japanese エコーバックの取得
    // \~english Gets the echoback
    n = connection_readline(&urg->connection,
                            buffer, SAFETY_BUFFER_SIZE, extended_timeout * 10);
    if (n <= 0) {
        return set_errno_and_return(urg, URG_NO_RESPONSE);
    }

	buffer[n] = '\0';

	if ((n < body_size) || (buffer[0] != stx)) {
        return set_errno_and_return(urg, URG_INVALID_RESPONSE);
    }

	if (safety_check_crc(buffer, n)) {
        return set_errno_and_return(urg, URG_CHECKSUM_ERROR);
    }

	if(n <= body_offset + crc_size){
		return 0;
	}

	if (status_data)
	{
		status_data->is_setting = buffer[body_offset] == '1' ? 1 : 0;

		strncpy(param, &buffer[body_offset +1], 2);
		param[2] = '\0';
		status_data->area_number = strtol(param, NULL, 16);

		status_data->is_error_detected = buffer[body_offset + 3] == '1' ? 1 : 0;

		strncpy(param, &buffer[body_offset + 4], 2);
		param[2] = '\0';
		status_data->error_code = strtol(param, NULL, 16);

		status_data->is_lockout = buffer[body_offset + 6] == '1' ? 1 : 0;

        status_data->is_ossd1_on = buffer[body_offset + 7] == '1' ? 1 : 0;
        status_data->is_ossd2_on = buffer[body_offset + 8] == '1' ? 1 : 0;

        status_data->is_warning1_on = buffer[body_offset + 9] == '1' ? 1 : 0;
        status_data->is_warning2_on = buffer[body_offset + 10] == '1' ? 1 : 0;

        status_data->is_ossd3_on = buffer[body_offset + 11] == '1' ? 1 : 0;
        status_data->is_ossd4_on = buffer[body_offset + 12] == '1' ? 1 : 0;

        status_data->reserved1 = buffer[body_offset + 13] == '1' ? 1 : 0;
        status_data->reserved2 = buffer[body_offset + 14] == '1' ? 1 : 0;

		status_data->reserved1 = buffer[body_offset + 13] == '1' ? 1 : 0;
		status_data->reserved2 = buffer[body_offset + 14] == '1' ? 1 : 0;

		status_data->is_mut_over1_on = buffer[body_offset + 15] == '1' ? 1 : 0;
		status_data->is_mut_over2_on = buffer[body_offset + 16] == '1' ? 1 : 0;

		status_data->is_reset_req1_on = buffer[body_offset + 17] == '1' ? 1 : 0;
		status_data->is_reset_req2_on = buffer[body_offset + 18] == '1' ? 1 : 0;

		strncpy(param, &buffer[body_offset +19], 4);
		param[4] = '\0';
		status_data->encoder_velocity = strtol(param, NULL, 16);

		status_data->is_laser_off = buffer[body_offset + 23] == '1' ? 1 : 0;

		status_data->is_slave1_ossd12_on = buffer[body_offset + 24] == '1' ? 1 : 0;
		status_data->is_slave2_ossd12_on = buffer[body_offset + 25] == '1' ? 1 : 0;
		status_data->is_slave3_ossd12_on = buffer[body_offset + 26] == '1' ? 1 : 0;

		status_data->is_slave1_ossd34_on = buffer[body_offset + 27] == '1' ? 1 : 0;
		status_data->is_slave2_ossd34_on = buffer[body_offset + 28] == '1' ? 1 : 0;
		status_data->is_slave3_ossd34_on = buffer[body_offset + 29] == '1' ? 1 : 0;

		status_data->is_slave1_warning1_on = buffer[body_offset + 30] == '1' ? 1 : 0;
		status_data->is_slave2_warning1_on = buffer[body_offset + 31] == '1' ? 1 : 0;
		status_data->is_slave3_warning1_on = buffer[body_offset + 32] == '1' ? 1 : 0;

		status_data->is_slave1_warning2_on = buffer[body_offset + 33] == '1' ? 1 : 0;
		status_data->is_slave2_warning2_on = buffer[body_offset + 34] == '1' ? 1 : 0;
		status_data->is_slave3_warning2_on = buffer[body_offset + 35] == '1' ? 1 : 0;

		status_data->is_slave1_error_detected = buffer[body_offset + 36] == '1' ? 1 : 0;
		status_data->is_slave2_error_detected = buffer[body_offset + 37] == '1' ? 1 : 0;
		status_data->is_slave3_error_detected = buffer[body_offset + 38] == '1' ? 1 : 0;

		status_data->is_slave1_laser_off = buffer[body_offset + 39] == '1' ? 1 : 0;
		status_data->is_slave2_laser_off = buffer[body_offset + 40] == '1' ? 1 : 0;
		status_data->is_slave3_laser_off = buffer[body_offset + 41] == '1' ? 1 : 0;

		strncpy(param, &buffer[body_offset + 42], 8);
		param[8] = '\0';
		status_data->timestamp = strtol(param, NULL, 16);

		status_data->is_optical_window_contamination_warning = buffer[body_offset + 50] == '1' ? 1 : 0;		//Compatible with version 2.1.0 onwards

        strncpy(param, &buffer[body_offset + 51], 1);
        param[1] = '\0';
        status_data->encoder_input_pattern_num = strtol(param, NULL, 16);

        strncpy(param, &buffer[body_offset + 52], 4);
        param[4] = '\0';
        status_data->encoder_angular_velocity = strtol(param, NULL, 16);

		for(i = 0; i < 34; i++){
			status_data->reserved[i] = buffer[body_offset + 56 + i];
		}
	}

	data_buffer = &buffer[body_offset + 92 + 8];

	urg->received_first_index = urg->first_data_index;
	urg->received_last_index = urg->last_data_index;
    urg->received_skip_step = 0;
    return n;
}

//! \~japanese 検出ログデータの取得  \~english Gets detection log data
static int safety_receive_detection_log_data(urg_t *urg, 
								 detection_log_data_t *detection_log_data)
{
	enum {
		MAX_DETECTION_LOG = 30,
		stx = 0x02,
        stx_size = 1,
		etx = 0x03,
		etx_size = 1,
        command_size = 4,
		body_size = 4,
        crc_size = 4,
		body_offset = 1 + 4 + 4 + 2,
    };
    char buffer[SAFETY_BUFFER_SIZE];
	char param[9];
	char *data_buffer = 0;
    int n, i;
    int extended_timeout = urg->timeout
        + 2 * (urg->scan_usec * (urg->scanning_skip_scan) / 1000);

    // \~japanese エコーバックの取得
    // \~english Gets the echoback
    n = connection_readline(&urg->connection,
                            buffer, SAFETY_BUFFER_SIZE, extended_timeout * 10);
    if (n <= 0) {
        return set_errno_and_return(urg, URG_NO_RESPONSE);
    }

	buffer[n] = '\0';

	if ((n < body_size) || (buffer[0] != stx)) {
        return set_errno_and_return(urg, URG_INVALID_RESPONSE);
    }

	if (safety_check_crc(buffer, n)) {
        return set_errno_and_return(urg, URG_CHECKSUM_ERROR);
    }

	if(n <= body_offset + crc_size){
		return 0;
	}

	data_buffer = &buffer[body_offset];

	if (detection_log_data)
	{
		for(i = 0; i < MAX_DETECTION_LOG; i++)
		{
			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].io.io_all = strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].protection1_min_dist = strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].protection1_min_dist_step = strtol(param, NULL, 16);
			data_buffer += 4;
			
			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].protection2_min_dist = strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].protection2_min_dist_step = strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].slave1_io.io_all = strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].reserved1 = (char)strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].reserved2 = (char)strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].slave2_io.io_all = strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].reserved3 = (char)strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].reserved4 = (char)strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].slave3_io.io_all = strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].reserved5 = (char)strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 4);
			param[4] = '\0';
			detection_log_data[i].reserved6 = (char)strtol(param, NULL, 16);
			data_buffer += 4;

			strncpy(param, data_buffer, 8);
			param[8] = '\0';
			detection_log_data[i].timestamp = (char)strtol(param, NULL, 16);
			data_buffer += 8;
		}
	}
    return n;
}

//! \~japanese 検出ログデータのクリア  \~english Clears detection log data
static int safety_receive_detection_log_clear(urg_t *urg)
{
	enum {
		stx = 0x02,
        stx_size = 1,
		etx = 0x03,
		etx_size = 1,
        command_size = 4,
		body_size = 4,
        crc_size = 4,
		body_offset = 1 + 4 + 4 + 2,
		status1 = 9,
		status2 = 10,
    };
    char buffer[BUFFER_SIZE];
	char *data_buffer = 0;
    int n = 0;
    int extended_timeout = urg->timeout
        + 2 * (urg->scan_usec * (urg->scanning_skip_scan) / 1000);

    // \~japanese エコーバックの取得
    // \~english Gets the echoback
    n = connection_readline(&urg->connection,
                            buffer, BUFFER_SIZE, extended_timeout * 10);
    if (n <= 0) {
        return set_errno_and_return(urg, URG_NO_RESPONSE);
    }

	buffer[n] = '\0';

	if ((n < body_size) || (buffer[0] != stx)) {
        return set_errno_and_return(urg, URG_INVALID_RESPONSE);
    }

	if (safety_check_crc(buffer, n)) {
        return set_errno_and_return(urg, URG_CHECKSUM_ERROR);
    }

	if(n < body_offset + crc_size){
		return n;
	}

	if((buffer[status1] != '0') || (buffer[status2] != '0'))
	{
		return 0;
	}
	return n;
}


int urg_get_distance(urg_t *urg, long data[], long *time_stamp)
{
    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }
    return receive_data(urg, data, NULL, time_stamp);
}

int safety_get_distance(urg_t *urg, long data[], safety_data_t *safety_data)
{
    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }
    return safety_receive_data(urg, data, NULL, safety_data);
}


int urg_get_distance_intensity(urg_t *urg,
                               long data[], unsigned short intensity[],
                               long *time_stamp)
{
    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    return receive_data(urg, data, intensity, time_stamp);
}

int safety_get_distance_intensity(urg_t *urg,
                               long data[], unsigned short intensity[],
                               safety_data_t *safety_data)
{
    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    return safety_receive_data(urg, data, intensity, safety_data);
}


int urg_get_multiecho(urg_t *urg, long data_multi[], long *time_stamp)
{
    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    return receive_data(urg, data_multi, NULL, time_stamp);
}


int urg_get_multiecho_intensity(urg_t *urg,
                                long data_multi[],
                                unsigned short intensity_multi[],
                                long *time_stamp)
{
    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    return receive_data(urg, data_multi, intensity_multi, time_stamp);
}


int urg_stop_measurement(urg_t *urg)
{
    enum { MAX_READ_TIMES = 3 };
    int ret = URG_INVALID_RESPONSE;
    int n;
    int i;

    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    // \~japanese  QT を発行する
    // \~english Sends the QT command
    n = connection_write(&urg->connection, "QT\n", 3);
    if (n != 3) {
        return set_errno_and_return(urg, URG_SEND_ERROR);
    }

    for (i = 0; i < MAX_READ_TIMES; ++i) {
        // \~japanese QT の応答が返されるまで、距離データを読み捨てる
        // \~english Skips measuement data until QT response is received
        ret = receive_data(urg, NULL, NULL, NULL);
        if (ret == URG_NO_ERROR) {
            // \~japanese 正常応答
	    // \~english Correct response
            urg->is_laser_on = URG_FALSE;
            urg->is_sending = URG_FALSE;
            return set_errno_and_return(urg, URG_NO_ERROR);
        }
    }
    return ret;
}

int safety_stop_measurement(urg_t *urg){
	enum {
		stx = 0x02,
        stx_size = 1,
		etx = 0x03,
		etx_size = 1,
        command_size = 4,
		body_size = 4,
        crc_size = 4,
    };
    char command [body_size + 1];
    int ret = 0;
	unsigned short crc = 0;
	int message_size = 0;

    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

	snprintf(command, body_size, "AR03");

	command[body_size] = '\0'; 

	return safety_send_command(urg, command);
}


int urg_set_scanning_parameter(urg_t *urg, int first_step, int last_step,
                               int skip_step)
{
    // \~japanese  設定の範囲外を指定したときは、エラーを返す
    // \~english If parameters are out of range, return an error
    if (((skip_step < 0) || (skip_step >= 100)) ||
        (first_step > last_step) ||
        (first_step < -urg->front_data_index) ||
        (last_step > (urg->last_data_index - urg->front_data_index))) {
        return set_errno_and_return(urg, URG_SCANNING_PARAMETER_ERROR);
    }

    urg->scanning_first_step = first_step;
    urg->scanning_last_step = last_step;
    urg->scanning_skip_step = skip_step;

    return set_errno_and_return(urg, URG_NO_ERROR);
}


int urg_set_connection_data_size(urg_t *urg,
                                 urg_range_data_byte_t data_byte)
{
    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    if ((data_byte != URG_COMMUNICATION_3_BYTE) || (data_byte != URG_COMMUNICATION_2_BYTE)) {
        return set_errno_and_return(urg, URG_DATA_SIZE_PARAMETER_ERROR);
    }

    urg->range_data_byte = data_byte;

    return set_errno_and_return(urg, URG_NO_ERROR);
}


int urg_laser_on(urg_t *urg)
{
    int expected[] = { 0, 2, EXPECTED_END };
    int ret;

    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    if (urg->is_laser_on != URG_FALSE) {
        // \~japanese  既にレーザが発光しているときは、コマンドを送信しないようにする
        // \~english If laser is already on don't send the command
        urg->last_errno = 0;
        return urg->last_errno;
    }

    ret = scip_response(urg, "BM\n", expected, urg->timeout, NULL, 0);
    if (ret >= 0) {
        urg->is_laser_on = URG_TRUE;
        ret = 0;
    }
    return ret;
}


int urg_laser_off(urg_t *urg)
{
    return urg_stop_measurement(urg);
}


int urg_reboot(urg_t *urg)
{
    int expected[] = { 0, 1, EXPECTED_END };
    int ret;
    int i;

    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }

    // \~japanese  ２回目の RB 送信後、接続を切断する
    // \~english After sending the 2nd RB then close the connection
    for (i = 0; i < 2; ++i) {
        ret = scip_response(urg, "RB\n", expected, urg->timeout, NULL, 0);
        if (ret < 0) {
            return set_errno_and_return(urg, URG_INVALID_RESPONSE);
        }
    }
    urg->is_active = URG_FALSE;
    urg_close(urg);

    urg->last_errno = 0;
    return urg->last_errno;
}


void urg_sleep(urg_t *urg)
{
    enum { RECEIVE_BUFFER_SIZE = 4 };
    int sl_expected[] = { 0, EXPECTED_END };
    char receive_buffer[RECEIVE_BUFFER_SIZE];

    if (urg_stop_measurement(urg) != URG_NO_ERROR) {
        return;
    }

    scip_response(urg, "%SL\n", sl_expected, MAX_TIMEOUT,
                  receive_buffer, RECEIVE_BUFFER_SIZE);
}


void urg_wakeup(urg_t *urg)
{
    urg_stop_measurement(urg);
}


int urg_is_stable(urg_t *urg)
{
    const char *stat = urg_sensor_status(urg);
    return strncmp("Stable", stat, 6) ? 0 : 1;
}


static char *copy_token(char *dest, char *receive_buffer,
                        const char *start_str, const char *end_ch, int lines)
{
    size_t start_str_len = strlen(start_str);
    size_t end_ch_len = strlen(end_ch);
    int i;
    size_t j;

    for (j = 0; j < end_ch_len; ++j) {
        const char *p = receive_buffer;

        for (i = 0; i < lines; ++i) {
            if (!strncmp(p, start_str, start_str_len)) {

                char *last_p = strchr(p + start_str_len, end_ch[j]);
                if (last_p) {
                    *last_p = '\0';
                    memcpy(dest, p + start_str_len,
                           last_p - (p + start_str_len) + 1);
                    return dest;
                }
            }
            p += strlen(p) + 1;
        }
    }
    return NULL;
}


static const char *receive_command_response(urg_t *urg,
                                            char *buffer, int buffer_size,
                                            const char* command,
                                            int response_lines)
{
    const int vv_expected[] = { 0, EXPECTED_END };
    int ret;

    if (!urg->is_active) {
        return NOT_CONNECTED_MESSAGE;
    }

    ret = scip_response(urg, command, vv_expected, urg->timeout,
                        buffer, buffer_size);
    if (ret < response_lines) {
        return RECEIVE_ERROR_MESSAGE;
    }

    return NULL;
}


const char *urg_sensor_product_type(urg_t *urg)
{
    enum {
        RECEIVE_BUFFER_SIZE = BUFFER_SIZE * VV_RESPONSE_LINES,
    };
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    const char *ret;
    char *p;

    ret = receive_command_response(urg, receive_buffer, RECEIVE_BUFFER_SIZE,
                                   "VV\n", VV_RESPONSE_LINES);
    if (ret) {
        return ret;
    }

    p = copy_token(urg->return_buffer,
                   receive_buffer, "PROD:", ";", VV_RESPONSE_LINES);
    return (p) ? p : RECEIVE_ERROR_MESSAGE;
}


const char *urg_sensor_serial_id(urg_t *urg)
{
    enum {
        RECEIVE_BUFFER_SIZE = BUFFER_SIZE * VV_RESPONSE_LINES,
    };
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    const char *ret;
    char *p;

    ret = receive_command_response(urg, receive_buffer, RECEIVE_BUFFER_SIZE,
                                   "VV\n", VV_RESPONSE_LINES);
    if (ret) {
        return ret;
    }

    p = copy_token(urg->return_buffer,
                   receive_buffer, "SERI:", ";", VV_RESPONSE_LINES);
    return (p) ? p : RECEIVE_ERROR_MESSAGE;
}


const char *urg_sensor_firmware_version(urg_t *urg)
{
    enum {
        RECEIVE_BUFFER_SIZE = BUFFER_SIZE * VV_RESPONSE_LINES,
    };
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    const char *ret;
    char *p;

    if (!urg->is_active) {
        return NOT_CONNECTED_MESSAGE;
    }

    ret = receive_command_response(urg, receive_buffer, RECEIVE_BUFFER_SIZE,
                                   "VV\n", VV_RESPONSE_LINES);
    if (ret) {
        return ret;
    }

    p = copy_token(urg->return_buffer,
                   receive_buffer, "FIRM:", ";", VV_RESPONSE_LINES);
    return (p) ? p : RECEIVE_ERROR_MESSAGE;
}

const char *safety_sensor_product_type(urg_t *urg)
{
    enum {
        RECEIVE_BUFFER_SIZE = 200,
		FIELD_OFFSET = 10,
    };
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    int ret, i;
    char *p;

    ret = safety_response(urg, "VR00", urg->timeout, receive_buffer, RECEIVE_BUFFER_SIZE);
    if (ret > 0) {
        i = 0;
		p = &receive_buffer[FIELD_OFFSET];
		while(*p != ' '){
			urg->return_buffer[i++] = *p++;
		}
		urg->return_buffer[i] = '\0';
		return urg->return_buffer;
    }
    return RECEIVE_ERROR_MESSAGE;
}


const char *safety_sensor_serial_id(urg_t *urg)
{
    enum {
        RECEIVE_BUFFER_SIZE = 200,
		FIELD_OFFSET = 108,
		FIELD_SIZE = 16,
    };
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    int ret, i;
    char *p;

    ret = safety_response(urg, "VR00", urg->timeout, receive_buffer, RECEIVE_BUFFER_SIZE);
    if (ret > 0) {
        i = 0;
		p = &receive_buffer[FIELD_OFFSET];
		while(i < FIELD_SIZE){
			urg->return_buffer[i++] = *p++;
		}
		urg->return_buffer[i] = '\0';
		return urg->return_buffer;
    }
    return RECEIVE_ERROR_MESSAGE;
}


const char *safety_sensor_firmware_version(urg_t *urg)
{
    enum {
        RECEIVE_BUFFER_SIZE = 200,
		FIELD_OFFSET = 40,
    };
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    int ret, i;
    char *p;

    ret = safety_response(urg, "VR00", urg->timeout, receive_buffer, RECEIVE_BUFFER_SIZE);
    if (ret > 0) {
        i = 0;
		p = &receive_buffer[FIELD_OFFSET];
		while(*p != ' '){
			urg->return_buffer[i++] = *p++;
		}
		urg->return_buffer[i] = '\0';
		return urg->return_buffer;
    }
    return RECEIVE_ERROR_MESSAGE;
}

const char *urg_sensor_status(urg_t *urg)
{
    enum {
        RECEIVE_BUFFER_SIZE = BUFFER_SIZE * II_RESPONSE_LINES,
    };
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    const char *ret;
    char *p;

    if (!urg->is_active) {
        return NOT_CONNECTED_MESSAGE;
    }

    ret = receive_command_response(urg, receive_buffer, RECEIVE_BUFFER_SIZE,
                                   "II\n", II_RESPONSE_LINES);
    if (ret) {
        return ret;
    }

    p = copy_token(urg->return_buffer,
                   receive_buffer, "STAT:", ";", II_RESPONSE_LINES);
    return (p) ? p : RECEIVE_ERROR_MESSAGE;
}


const char *urg_sensor_state(urg_t *urg)
{
    enum {
        RECEIVE_BUFFER_SIZE = BUFFER_SIZE * II_RESPONSE_LINES,
    };
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    const char *ret;
    char *p;

    if (!urg->is_active) {
        return NOT_CONNECTED_MESSAGE;
    }

    ret = receive_command_response(urg, receive_buffer, RECEIVE_BUFFER_SIZE,
                                   "II\n", II_RESPONSE_LINES);
    if (ret) {
        return ret;
    }

    p = copy_token(urg->return_buffer,
                   receive_buffer, "MESM:", " (", II_RESPONSE_LINES);
    return (p) ? p : RECEIVE_ERROR_MESSAGE;
}


void urg_set_error_handler(urg_t *urg, urg_error_handler handler)
{
    urg->error_handler = handler;
}

int safety_get_status(urg_t *urg, status_data_t *status_data)
{
    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }
    return safety_receive_status_data(urg, status_data);
}

int safety_get_area(urg_t *urg, long data[])
{
    if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }
    return safety_receive_area_data(urg, data);
}

int safety_get_detection_log(urg_t *urg, detection_log_data_t *detection_log_data)
{
	 if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }
    return safety_receive_detection_log_data(urg, detection_log_data);
}

int safety_get_detection_log_clear(urg_t *urg)
{
	 if (!urg->is_active) {
        return set_errno_and_return(urg, URG_NOT_CONNECTED);
    }
    return safety_receive_detection_log_clear(urg);
}