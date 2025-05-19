/*
	Sample program to obtain area data in the sensor using YR Command.
	
	Normal execution (default connection)
	> get_area_safetyCMD.exe
	
	Execution with spcific IP address
	> get_area_safetyCMD.exe 192.168.0.11
	
	
	要求コマンドによりセンサからエリアを取得するサンプルプログラム（YRコマンド使用）
	
	通常実行（デフォルト接続先）
	> get_area_safetyCMD.exe

	IPアドレス指定実行
	> get_area_safetyCMD.exe 192.168.0.11
*/

#include "safety_sensor_safetyCMD.h"
#include <stdio.h>
#include <signal.h>

#define TRUE (1)
#define FALSE (0)


#define ALL_OUTPUT

/*
	\
	\brief Show area data  
	エリアデータ表示

	Function to show area data.
	エリアデータを表示します。

	\param[in] safety_data  Area Data   
						     エリアデータ
*/
static void print_info(area_cmd_param_t *area_cmd_param, long area[])
{
	enum{
		area_number_size = 2,
		step_size = 4,
		grouping_size = 2,
		display_offset = 1,
		area_cast = 0x7FFF,
	};
	char area_type[9][17] = {"Protection 1", "Protection 2",
	                         "Warning 1", "Warning2",
	                         "Muting 1", "Muting 2",
					         "Reference center", "Reference max", "Reference min"};

	int i;
	int cnt = 0;
	int number_of_steps = 0;
	int remaining_steps = 0;
	int total_steps = 0;
	int center_step = 0;
	int center_step_data = 0;
	char now_area_type[17] = {0};
	

	// Area Type is specifed by numbers 0 to 8. It is converted into string for the display.
	// エリアタイプは0～8で電文作成されるため、出力時は文字列に変換する。
	sprintf(now_area_type, "%s", area_type[area_cmd_param->area_type]);
	
	// Display information 
	// 各種情報表示
	printf("Area Type               : %s\n", now_area_type);
	//Area is specified by numbers 0 to 31. It is matched to 7 seg display number by adding 1.
	// エリアは0～31で電文作成されるため、出力時は+1することで7seg表示と合わせる
	printf("Area Number             : %0*d\n", area_number_size, area_cmd_param->area_number + display_offset); 
	printf("Area Start Step         : %0*d\n", step_size, area_cmd_param->start_step);
	printf("Area End Step           : %0*d\n", step_size, area_cmd_param->end_step);
	printf("Area Step Grouping      : %0*d\n", grouping_size, area_cmd_param->grouping);
	

	cnt = area_cmd_param->end_step - area_cmd_param->start_step;
	number_of_steps = (int)(cnt/area_cmd_param->grouping);
	remaining_steps = cnt % area_cmd_param->grouping;
	total_steps = number_of_steps + remaining_steps;
	center_step = area_cmd_param->start_step + (int)(cnt/2);
	center_step_data = (int)(total_steps/2);

#ifdef ALL_OUTPUT
	for (i =  0; i < number_of_steps; i++)
	{
		printf("Data %4d : Area Range : %6ld\n", i , (area[i] & area_cast));
	}
	if(remaining_steps > 0){
		printf("Data %4d : Area Range : %6ld\n",  cnt, (area[i] & area_cast));
	}
#else
	
	// Show only the center step area. 
	// 表示ステップを中央ステップのみに限定
	printf("Step %4d : Area Range : %6ld\n", center_step, (area[center_step_data] & area_cast));
#endif

	printf("\n");
}


static int get_area (sensor_t *sensor, area_cmd_param_t *area_cmd_param)
{
	enum
	{
		MAX_STEP_SIZE = 1081
	};
	char functions[9][17] = {"Protection 1", "Protection 2",
	                         "Warning 1", "Warning 2",
	                         "Muting 1", "Muting 2",
					         "Reference", "Reference", "Reference"};
	long area[MAX_STEP_SIZE];
    int n;

	//Transmit YR command
	// YRコマンド送信
	if (safety_sensor_request_area_safetyCMD(sensor, area_cmd_param) < 0)
	{
	    safety_sensor_close_safetyCMD(sensor);
		return 2;
	}

	//Receive area
	// エリア値受信
	n = safety_sensor_get_area_safetyCMD(sensor, area);

	if (n <= 0)
	{
		printf("safety_sensor_get_area_safetyCMD() error.\n" "%s function may not be active.\n\n", functions[area_cmd_param->area_type]);
	}
	else
	{
		//Dispaly
		// 表示
		print_info(area_cmd_param, area);
	}
	return n;
}

int main(int argc, char *argv[])
{
	enum {
		START_STEP = 0,
		END_STEP = 1080,
		GROUP_STEP = 1,
		MAX_STEP_SIZE = 1081,
    };
	enum {
		PROTECTION_1 = 0,
		PROTECTION_2,
		WARNING_1,
		WARNING_2,
		MUTING_1, 
		MUTING_2,
		REFERENCE_CENTER,
		REFERENCE_MAX,
		REFERENCE_MIN,
	};
    sensor_t sensor;
	area_cmd_param_t area_cmd_param;
   
	// Connect
	// 接続
	if (safety_sensor_open_safetyCMD(&sensor, argc, argv) < 0) {
        return 1;
    }

	// Transmit VR00 Command to obtain the serial number
	// VR00コマンドによりシリアル番号取得
	printf("Sensor serial ID : %s\n\n", safety_sensor_serial_id_safetyCMD(&sensor));

	//Set the common parameter
	//共通パラメータセット
	area_cmd_param.area_number = 0;				//Area1
	area_cmd_param.start_step = START_STEP;
	area_cmd_param.end_step = END_STEP;
	area_cmd_param.grouping = GROUP_STEP;

	//Obtain Protection1 area 
	//防護１エリア取得
	area_cmd_param.area_type = PROTECTION_1;
	get_area(&sensor, &area_cmd_param);

	//Obtain Protection2 area (Dual protection mode should be active to obtain the data)
	//防護２エリア取得（デュアル防護領域を有効な場合のみデータ取得可能）
	area_cmd_param.area_type = PROTECTION_2;
	get_area(&sensor, &area_cmd_param);

	//Obtain Warning1 area (Warning1 area should be active to obtain the data)
	//警報１エリア取得（警報領域を有効な場合のみデータ取得可能）
	area_cmd_param.area_type = WARNING_1;
	get_area(&sensor, &area_cmd_param);

	//Obtain Warning2 area (Warning2 area should be active to obtain the data)
	//警報２エリア取得 (警報領域を有効な場合のみデータ取得可能)
	area_cmd_param.area_type = WARNING_2;
	get_area(&sensor, &area_cmd_param);

	//Obtain Muting1 area (Muting1 function should be active to obtain the data)
	//ミューティング１エリアを取得(ミューティング1機能を有効な場合のみデータ取得可能)
	area_cmd_param.area_type = MUTING_1;
	get_area(&sensor, &area_cmd_param);

	//Obtain Muting2 area (Muting1 function should be active to obtain the data)
	//ミューティング２エリアを取得(ミューティング２機能を有効な場合のみデータ取得可能)
	area_cmd_param.area_type = MUTING_2;
	get_area(&sensor, &area_cmd_param);

	//Obtain Reference area center value (Reference function should be active to obtain the data)
	//リファレンスエリアの中心値を取得（リファレンス機能を有効な場合のみデータ取得可能）
	area_cmd_param.area_type = REFERENCE_CENTER;
	get_area(&sensor, &area_cmd_param);

	//Obtain Reference area max value (Reference function should be active to obtain the data)
	//リファレンスエリアの最大値を取得（リファレンス機能を有効な場合のみデータ取得可能）
	area_cmd_param.area_type = REFERENCE_MAX;
	get_area(&sensor, &area_cmd_param);

	//Obtain Reference area min value (Reference function should be active to obtain the data)
	//リファレンスエリアの最小値を取得（リファレンス機能を有効な場合のみデータ取得可能）
	area_cmd_param.area_type = REFERENCE_MIN;
	get_area(&sensor, &area_cmd_param);

	//Disconnect
	// 切断
    safety_sensor_close_safetyCMD(&sensor);

#if defined(URG_MSC)
    getchar();
#endif
    return 0;
}
