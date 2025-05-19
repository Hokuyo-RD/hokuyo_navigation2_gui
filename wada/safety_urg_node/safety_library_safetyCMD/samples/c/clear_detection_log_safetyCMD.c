/*
	Sample program to clear detection log data from the sensor using DC00 Command.
		
	Normal execution (default connection)
	> clear_detection_log_safetyCMD.exe
	
	Execution with spcific IP address
	> clear_detection_log_safetyCMD.exe 192.168.0.11
	
	要求コマンドによりセンサの検出ログをクリアするサンプルプログラム（DC00コマンド使用）
	
	通常実行（デフォルト接続先）
	> clear_detection_log_safetyCMD.exe

	IPアドレス指定実行
	> clear_detection_log_safetyCMD.exe 192.168.0.11
*/

#include "safety_sensor_safetyCMD.h"
#include <stdio.h>
#include <signal.h>

#define TRUE (1)
#define FALSE (0)

int main(int argc, char *argv[])
{
	sensor_t sensor;
    int n;

	// Connect
	// 接続
	if (safety_sensor_open_safetyCMD(&sensor, argc, argv) < 0) {
        return 1;
    }

	// Transmit VR00 Command to obtain the serial number
	// VR00コマンドによりシリアル番号取得
	printf("Sensor serial ID : %s\n\n", safety_sensor_serial_id_safetyCMD(&sensor));

	// Transmit DC00 command
	// DC00コマンド送信
	if (safety_sensor_request_detection_log_clear_safetyCMD(&sensor) < 0)
	{
	    safety_sensor_close_safetyCMD(&sensor);
		return 2;
	}

	// Receive the reply
	//受信
	n = safety_sensor_get_detection_log_clear_safetyCMD(&sensor);

	if (n <= 0)
	{
		printf("safety_sensor_get_detection_log_safetyCMD() error.\n");
	}
	else
	{
		// Display
		// 表示
		printf("safety_sensor_get_detection_log_safetyCMD() success.\n");
		printf("Detection log is cleared from the device.\n");
	}
	
	// Disconnect
	// 切断
	safety_sensor_close_safetyCMD(&sensor);

#if defined(URG_MSC)
    getchar();
#endif
    return 0;
}
