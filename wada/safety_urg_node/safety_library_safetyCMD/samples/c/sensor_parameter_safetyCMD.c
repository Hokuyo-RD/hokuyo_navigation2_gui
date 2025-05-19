/*
	Sample program to obtain sensor parameters using VR00 Command.
		
	Normal execution (default connection)
	> sensor_parameter_safetyCMD.exe
	
	Execution with spcific IP address
	> sensor_parameter_safetyCMD.exe 192.168.0.11
	
	要求コマンドによりセンサ情報を取得するサンプルプログラム

	通常実行（デフォルト接続先）
	> sensor_parameter_safetyCMD.exe

	IPアドレス指定実行
	> sensor_parameter_safetyCMD.exe 192.168.0.11
*/

#include "safety_sensor_safetyCMD.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    sensor_t sensor;

	// Connect
	// 接続
	if (safety_sensor_open_safetyCMD(&sensor, argc, argv) < 0) {
		return 1;
	}

	// VR00 Command
	// VR00コマンド
	printf("Sensor product type     : %s\n", safety_sensor_product_type_safetyCMD(&sensor));
	printf("Sensor firmware version : %s\n", safety_sensor_firmware_version_safetyCMD(&sensor));
	printf("Sensor serial ID        : %s\n", safety_sensor_serial_id_safetyCMD(&sensor));

	// Disconnect
	// 切断
	safety_sensor_close_safetyCMD(&sensor);

#if defined(URG_MSC)
    getchar();
#endif
    return 0;
}
