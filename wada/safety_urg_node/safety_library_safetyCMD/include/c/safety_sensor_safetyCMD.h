#ifndef SAFETY_SENSOR_SAFETYCMD_H
#define SAFETY_SENSOR_SAFETYCMD_H

/*
	センサへの要求コマンド関数を提供します。
	お使いのファームウェアバージョンで非対応の関数を使用された場合の動作保障はできません。
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "safety_param.h"

/*
	\brief 接続

	指定したIPアドレスのセンサに接続し、コマンド送信を可能な状態にします。
	IPアドレスの指定がない場合、デフォルトの接続先（192.168.0.10）になります。
	ライブラリの他の関数を呼び出す前に、この関数を呼び出す必要があります。

	センサファームウェアバージョン01.00.00以降対応

	\param[in, out] sensor センサ管理情報
	\param[in] argc プログラム実行時の引数
	\param[in] argv プログラム実行時の引数

	\retval 0 接続完了
	\retval <0 接続失敗
*/
extern int safety_sensor_open_safetyCMD(sensor_t *sensor, int argc, char *argv[]);

/*
	\brief 切断

	接続しているセンサから切断します。
	プログラム終了時に、この関数を呼び出す必要があります。

	センサファームウェアバージョン01.00.00以降対応

	\param[in, out] sensor センサ管理情報
*/
extern void safety_sensor_close_safetyCMD(sensor_t *sensor);

/*
	\brief VR00コマンド センサ型式

	VR00コマンドを送信し、センサ型式を取得します。

	センサファームウェアバージョン01.00.00以降対応

	\param[in, out] sensor センサ管理情報

	\return センサ型式の文字列
*/
extern const char *safety_sensor_product_type_safetyCMD(sensor_t *sensor);

/*
	\brief VR00コマンド ファームウェアバージョン

	VR00コマンドを送信し、ファームウェアバージョンを取得します。

	センサファームウェアバージョン01.00.00以降対応

	\param[in, out] sensor センサ管理情報

	\return ファームウェアバージョンの文字列
*/
extern const char *safety_sensor_firmware_version_safetyCMD(sensor_t *sensor);

/*
	\brief VR00コマンド シリアル番号

	VR00コマンドを送信し、シリアル番号を取得します。

	センサファームウェアバージョン01.00.00以降対応

	\param[in, out] sensor センサ管理情報

	\return シリアル番号の文字列
*/
extern const char *safety_sensor_serial_id_safetyCMD(sensor_t *sensor);

/*
	\brief AR00送信 距離値計測開始

	AR00コマンドを送信します。
	受信に関しては、距離値取得関数で行います。

	センサファームウェアバージョン01.00.00以降対応

	\param[in, out] sensor センサ管理情報

	\retval 0 送信成功
	\retval <0 送信失敗
*/
extern int safety_sensor_request_distance_handshaking_safetyCMD(sensor_t *sensor);

/*
	\brief AR00/AR02 距離値取得

	センサから、距離値を取得します。
	AR00/AR02共通の取得用コマンドです。

	センサファームウェアバージョン01.00.00以降対応

	\param[in, out] sensor センサ管理情報
	\param[out] distance 距離値データ[mm]
	\param[out] safety_data 安全データ

	\retval >0 取得した距離値データ数
	\retval <=0 取得失敗
*/
extern int safety_sensor_get_distance_safetyCMD(sensor_t *sensor, long *distance, safety_data_t *safety_data);

/*
	\brief AR01送信 距離値レベル値計測開始

	AR01コマンドを送信します。
	受信に関しては、距離値レベル値取得関数で行います。

	センサファームウェアバージョン01.00.00以降対応

	\param[in, out] sensor センサ管理情報

	\retval 0 送信成功
	\retval <0 送信失敗
*/
extern int safety_sensor_request_distance_intensity_handshaking_safetyCMD(sensor_t *sensor);

/*
	\brief AR01/AR04 距離値レベル値取得

	センサから、距離値とレベル値を取得します。
	AR01/AR04共通の取得用コマンドです。

	センサファームウェアバージョン01.00.00以降対応
	
	\param[in, out] sensor センサ管理情報
	\param[out] distance 距離値データ[mm]
	\param[out] intensity レベル値データ
	\param[out] safety_data 安全データ

	\retval >0 取得した距離値レベル値データ数
	\retval <=0 取得失敗
*/
extern int safety_sensor_get_distance_intensity_safetyCMD(sensor_t *sensor, long *distance, unsigned short *intensity, safety_data_t *safety_data);

/*
	\brief AR02送信 距離値計測開始（垂れ流し）

	AR02コマンドを送信します。
	受信に関しては、距離値取得関数で行います。

	センサファームウェアバージョン01.00.00、および01.00.03以降対応（01.00.02は非対応）

	\param[in, out] sensor センサ管理情報

	\retval 0 送信成功
	\retval <0 送信失敗
*/
extern int safety_sensor_request_distance_continuous_safetyCMD(sensor_t *sensor);

/*
	\brief AR03送信 AR02垂れ流し停止

	AR03コマンドを送信します。
	AR02による垂れ流しモードを停止します。

	センサファームウェアバージョン01.00.00、および01.00.03以降対応（01.00.02は非対応）

	\param[in, out] sensor センサ管理情報

	\retval 0 停止成功
	\retval <0 停止失敗
*/
extern int safety_sensor_stop_measurement_distance_safetyCMD(sensor_t *sensor);

/*
	\brief AR04送信 距離値レベル値計測開始（垂れ流し）

	AR04コマンドを送信します。
	受信に関しては、距離値レベル値取得関数で行います。

	センサファームウェアバージョン01.00.00、および01.00.03以降対応（01.00.02は非対応）

	\param[in, out] sensor センサ管理情報

	\retval 0 送信成功
	\retval <0 送信失敗
*/
extern int safety_sensor_request_distance_intensity_continuous_safetyCMD(sensor_t *sensor);

/*
	\brief AR05送信 AR04垂れ流し停止

	AR05コマンドを送信します。
	AR04による垂れ流しモードを停止します。

	センサファームウェアバージョン01.00.00、および01.00.03以降対応（01.00.02は非対応）

	\param[in, out] sensor センサ管理情報

	\retval 0 停止成功
	\retval <0 停止失敗
*/
extern int safety_sensor_stop_measurement_distance_intensity_safetyCMD(sensor_t *sensor);

/*
	\brief AR06送信 距離値計測開始

	AR06コマンドを送信します。
	受信に関しては、距離値取得関数で行います。

	センサファームウェアバージョン02.02.00以降対応

	\param[in, out] sensor センサ管理情報

	\retval 0 送信成功
	\retval <0 送信失敗
*/
extern int safety_sensor_request_fullstep_distance_handshaking_safetyCMD(sensor_t *sensor);

/*
	\brief AR07送信 距離値計測開始（垂れ流し）

	AR07コマンドを送信します。
	受信に関しては、距離値取得関数で行います。

	センサファームウェアバージョン02.02.00以降対応

	\param[in, out] sensor センサ管理情報

	\retval 0 送信成功
	\retval <0 送信失敗
*/
extern int safety_sensor_request_fullstep_distance_continuous_safetyCMD(sensor_t *sensor);

/*
	\brief AR08送信 AR07垂れ流し停止

	AR08コマンドを送信します。
	AR07による垂れ流しモードを停止します。

	センサファームウェアバージョン01.00.00、および01.00.03以降対応（01.00.02は非対応）

	\param[in, out] sensor センサ管理情報

	\retval 0 停止成功
	\retval <0 停止失敗
*/
extern int safety_sensor_stop_measurement_fullstep_distance_safetyCMD(sensor_t *sensor);

/*
	\brief XR00送信 

	XR00コマンドを送信します。
	センサのステータスを取得します。

	センサファームウェアバージョン2.00.00以降対応

	\param[in, out] sensor センサ管理情報

	\retval 0 停止成功
	\retval <0 停止失敗
*/
extern int safety_sensor_request_status_safetyCMD(sensor_t *sensor);

/*
	\brief XR00 ステータス取得

	センサから、ステータス情報を取得します。

	センサファームウェアバージョン02.00.00以降対応

	\param[in, out] sensor センサ管理情報
	\param[out] status_data ステータスデータ

	\retval >0 取得したデータ数
	\retval <=0 取得失敗
*/

extern int safety_sensor_get_status_safetyCMD(sensor_t *sensor, status_data_t *status_data);

/*
	\brief YR送信 エリア情報

	YRコマンドを送信します。
	受信に関しては、エリア取得関数で行います。

	センサファームウェアバージョン02.00.00以降対応

	\param[in, out] sensor センサ管理情報
	\param[in, out] area_cmd_param コマンドパラメータ情報
	

	\retval 0 送信成功
	\retval <0 送信失敗
*/
extern int safety_sensor_request_area_safetyCMD(sensor_t *sensor, area_cmd_param_t *area_cmd_param);

/*
	\brief YR エリア値取得

	センサから、エリア情報を取得します。

	センサファームウェアバージョン02.00.00以降対応

	\param[in, out] sensor センサ管理情報
	\param[in, out] エリアデータ

	\retval >0 取得したデータ数
	\retval <=0 取得失敗
*/
extern int safety_sensor_get_area_safetyCMD(sensor_t *sensor, long *area);


/*
	\brief DL00送信 

	DL00コマンドを送信します。
	センサの検出ログを取得します。

	センサファームウェアバージョン2.01.01以降対応

	\param[in, out] sensor センサ管理情報

	\retval 0 停止成功
	\retval <0 停止失敗
*/
extern int safety_sensor_request_detection_log_safetyCMD(sensor_t *sensor);

/*
	\brief DL00 検出ログ取得

	センサから、検出情報を取得します。
1
	センサファームウェアバージョン02.01.01以降対応

	\param[in, out] sensor センサ管理情報
	\param[out] detection_log_data 検出ログデータ

	\retval >0 取得したデータ数
	\retval <=0 取得失敗
*/
extern int safety_sensor_get_detection_log_safetyCMD(sensor_t *sensor, detection_log_data_t *detection_log_data);

/*
	\brief DC00送信 

	DC00コマンドを送信します。
	センサの検出ログをクリアします。

	センサファームウェアバージョン2.01.01以降対応

	\param[in, out] sensor センサ管理情報

	\retval 0 停止成功
	\retval <0 停止失敗
*/
extern int safety_sensor_request_detection_log_clear_safetyCMD(sensor_t *sensor);

/*
	\brief DC00 検出ログクリア

	センサの検出ログをクリアします。
1
	センサファームウェアバージョン02.01.01以降対応

	\param[in, out] sensor センサ管理情報

	\retval >0 取得したデータ数
	\retval <=0 取得失敗
*/
extern int safety_sensor_get_detection_log_clear_safetyCMD(sensor_t *sensor);

#ifdef __cplusplus
}
#endif

#endif /* !SAFETY_SENSOR_SAFETYCMD_H */
