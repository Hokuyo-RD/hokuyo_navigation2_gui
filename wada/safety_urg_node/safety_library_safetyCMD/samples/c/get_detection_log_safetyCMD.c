/*
	Sample program to obtain detection log data from the sensor using DL00 Command.
		
	Normal execution (default connection)
	> get_detection_log_safetyCMD.exe
	
	Execution with spcific IP address
	> get_detection_log_safetyCMD.exe 192.168.0.11
	
	要求コマンドによりセンサから検出ログを取得するサンプルプログラム（DL00コマンド使用）
	
	通常実行（デフォルト接続先）
	> get_detection_log_safetyCMD.exe

	IPアドレス指定実行
	> get_detection_log_safetyCMD.exe 192.168.0.11
*/

#include "safety_sensor_safetyCMD.h"
#include <stdio.h>
#include <signal.h>

#define TRUE (1)
#define FALSE (0)
#define ALL_OUTPUT


 enum{
		MAX_LOG_COUNT = 30,
	};

/*
	\brief Display the detection log data
		   検出ログデータ表示

	Displays the detection log data
	検出ログデータを表示します。

	\param[in] detection_log_data　Detection log data
								   検出ログデータ
*/
static void print_info(detection_log_data_t *detection_log_data)
{
	int i, cnt;
	
	// Dispaly the obtained information
	// 各種情報表示
#ifdef ALL_OUTPUT
	for(i = 0; i < MAX_LOG_COUNT; i++){
		printf("Detection log %4d\n", (i+1));
		printf("Area Number                 : %4d\n", detection_log_data[i].io.area);
		printf("Protection1 State           : %s\n", detection_log_data[i].io.protection1_state ? "On(Not detected)" : "Off(Detected)" );
		printf("Protection2 State           : %s\n", detection_log_data[i].io.protection2_state ? "On(Not detected)" : "Off(Detected)" );
		printf("Protection1 Min dist        : %4d\n", detection_log_data[i].protection1_min_dist);
		printf("Protection1 Min dist step   : %4d\n", detection_log_data[i].protection1_min_dist_step);
		printf("Protection2 Min dist        : %4d\n", detection_log_data[i].protection2_min_dist);
		printf("Protection2 Min dist step   : %4d\n", detection_log_data[i].protection2_min_dist_step);
		
		printf("Slave1 area number          : %4d\n", detection_log_data[i].slave1_io.area);
		printf("Slave1 Protection1 State    : %s\n", detection_log_data[i].slave1_io.protection1_state ? "On(Not detected)": "Off(Detected)");
		printf("Slave1 Protection2 State    : %s\n", detection_log_data[i].slave1_io.protection2_state ? "On(Not detected)": "Off(Detected)");
		
		printf("Slave1 area number          : %4d\n", detection_log_data[i].slave1_io.area);
		printf("Slave2 Protection1 State    : %s\n", detection_log_data[i].slave2_io.protection1_state ? "On(Not detected)": "Off(Detected)");
		printf("Slave2 Protection2 State    : %s\n", detection_log_data[i].slave2_io.protection2_state ? "On(Not detected)": "Off(Detected)");
		
		printf("Slave3 area number          : %4d\n", detection_log_data[i].slave3_io.area);
		printf("Slave3 Protection1 State    : %s\n", detection_log_data[i].slave3_io.protection1_state ? "On(Not detected)": "Off(Detected)");
		printf("Slave3 Protection2 State    : %s\n", detection_log_data[i].slave3_io.protection2_state ? "On(Not detected)": "Off(Detected)");

		printf("Log recorded                : %8ld seconds ago \n\n\n", detection_log_data[i].timestamp);
	}
#else
	cnt = 1;
	for(i = 0; i < MAX_LOG_COUNT; i++){
		if(detection_log_data[i].io.io_all != 0xFFFF){
			printf("Detection log %4d\n", (cnt++));
			printf("Area Number                 : %4d\n", detection_log_data[i].io.area);
			printf("Protection1 State           : %s\n", detection_log_data[i].io.protection1_state ? "On(Not detected)" : "Off(Detected)" );
			printf("Protection2 State           : %s\n", detection_log_data[i].io.protection2_state ? "On(Not detected)" : "Off(Detected)" );
			printf("Protection1 Min dist        : %4d\n", detection_log_data[i].protection1_min_dist);
			printf("Protection1 Min dist step   : %4d\n", detection_log_data[i].protection1_min_dist_step);
			printf("Protection2 Min dist        : %4d\n", detection_log_data[i].protection2_min_dist);
			printf("Protection2 Min dist step   : %4d\n", detection_log_data[i].protection2_min_dist_step);
		
			printf("Slave1 area number          : %4d\n", detection_log_data[i].slave1_io.area);
			printf("Slave1 Protection1 State    : %s\n", detection_log_data[i].slave1_io.protection1_state ? "On(Not detected)": "Off(Detected)");
			printf("Slave1 Protection2 State    : %s\n", detection_log_data[i].slave1_io.protection2_state ? "On(Not detected)": "Off(Detected)");
		
			printf("Slave1 area number          : %4d\n", detection_log_data[i].slave1_io.area);
			printf("Slave2 Protection1 State    : %s\n", detection_log_data[i].slave2_io.protection1_state ? "On(Not detected)": "Off(Detected)");
			printf("Slave2 Protection2 State    : %s\n", detection_log_data[i].slave2_io.protection2_state ? "On(Not detected)": "Off(Detected)");
		
			printf("Slave3 area number          : %4d\n", detection_log_data[i].slave3_io.area);
			printf("Slave3 Protection1 State    : %s\n", detection_log_data[i].slave3_io.protection1_state ? "On(Not detected)": "Off(Detected)");
			printf("Slave3 Protection2 State    : %s\n", detection_log_data[i].slave3_io.protection2_state ? "On(Not detected)": "Off(Detected)");

			printf("Log recorded                : %8ld seconds ago \n\n\n", detection_log_data[i].timestamp);
		}
	}
	if(cnt == 1){
		printf("There are no detection log data in the device!\n");
	}
#endif

	printf("\n");
}


int main(int argc, char *argv[])
{
	sensor_t sensor;
	detection_log_data_t detection_log_data[MAX_LOG_COUNT];
    int n;

	// Connect
	// 接続
	if (safety_sensor_open_safetyCMD(&sensor, argc, argv) < 0) {
        return 1;
    }

	// Transmit VR00 Command to obtain the serial number
	// VR00コマンドによりシリアル番号取得
	printf("Sensor serial ID : %s\n\n", safety_sensor_serial_id_safetyCMD(&sensor));

	// Transmit DL00 command
	// DL00コマンド送信
	if (safety_sensor_request_detection_log_safetyCMD(&sensor) < 0)
	{
	    safety_sensor_close_safetyCMD(&sensor);
		return 2;
	}

	// Receive the status data
	//ステータス受信
	n = safety_sensor_get_detection_log_safetyCMD(&sensor,&detection_log_data[0]);
	 
	if (n <= 0)
	{
		printf("safety_sensor_get_detection_log_safetyCMD() error.\n");
	}
	else
	{
		// Display
		// 表示
		print_info(detection_log_data);
	}
	
	// Disconnect
	// 切断
	safety_sensor_close_safetyCMD(&sensor);

#if defined(URG_MSC)
    getchar();
#endif
    return 0;
}
