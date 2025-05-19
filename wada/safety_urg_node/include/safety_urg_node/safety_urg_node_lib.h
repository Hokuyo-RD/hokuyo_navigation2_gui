#ifndef SAFETY_URG_NODE
#define SAFETY_URG_NODE


#ifdef DEBUG_MODE
#define DEBUG_PRINT(A) std::cout << (#A) << " : " << (A) << std::endl;
#else
#define DEBUG_PRINT(A) {;}
#endif

#include <chrono>
#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <thread>
#include <functional>
#include <limits>
#include <csignal>

#include <ros/ros.h>

#include <safety_data_message/SafetyData.h>
#include <sensor_msgs/LaserScan.h>

#include "safety_sensor_safetyCMD.h"
#include "private_functions.h"
#include "urg_sensor.h"
#include "urg_errno.h"
#include "urg_utils.h"
#include "safety_crc.h"

class SafetyUrgNode {
  private:
    /** ROS1 ハンドラ */
    ros::NodeHandle nh;
    ros::NodeHandle pnh;
    
    /** スレッドの終了フラグ */
    bool close_thread_;
    /** スキャンスレッドのスレッド変数 */
    std::thread scan_thread_;
    
    /** スキャンデータのpublisher */
    ros::Publisher scan_pub_;
    ros::Publisher safety_pub_;
    
    /** LiDAR管理構造体 */
    sensor_t sensor_;
    
    /** パラメータ一覧 */
    /** パラメータ"scan_topic" : スキャンデータ出力トピック名 */
    std::string scan_topic_;
    /** パラメータ"safety_topic" : Safetyデータ出力トピック名 */
    std::string safety_topic_;
    /** パラメータ"ip_address" : 接続先IPアドレス */
    std::string ip_address_;
    /** パラメータ"ip_port" : 接続ポート */
    int ip_port_;
    /** パラメータ"frame_id" : スキャンデータのframe_id */
    std::string frame_id_;
    /** パラメータ"calibrate_time" : 調整モード */
    bool calibrate_time_;
    /** パラメータ"synchronize_time" : 同期モード */
    bool synchronize_time_;
    /** パラメータ"error_limit" : 再接続を行うエラー回数 */
    int error_limit_;
    /** パラメータ"error_reset_period" : エラーをリセットする期間 */
    double error_reset_period_;
    /** パラメータ"continuous_mode" : 距離取得の 垂れ流し/ハンドシェイク を切り替える */
    bool continuous_mode_;
  
    /** トピックのframe_id設定 */
    std::string header_frame_id_;

    /** 通信エラーカウンタ（再接続時にリセット） */
    int error_count_;
    /** 通信エラー合計カウンタ（Active遷移時にリセット） */
    int total_error_count_;
    /** 再接続カウンタ（Active&Inactive時） */
    int reconnect_count_;

    /** LiDAR接続状態 */
    bool is_connected_;
    /** LiDAR計測状態 */
    bool is_measurement_started_;

    enum
    {
      MAX_NO_RECEIVE_COUNT = 5,
      MAX_STEP_SIZE = 1081
    };
    
    /**
     * @brief 初期化
     * @details 各種パラメータの設定、内部変数の初期化およびスキャンデータのpublisherの設定を行う
     */
    void initialize(void);
    
    /**
     * @brief LiDAR接続
     * @details LiDARとの接続処理とLiDAR情報の取得を行う
     * @retval true 接続成功
     * @retval false 接続失敗
     */
    bool connect(void);
    
    /**
     * @brief スキャン設定
     * @details 受信用のデータ領域確保やLiDARに対してスキャンの設定を行う
     */
    void set_scan_parameter(void);
    
    /**
     * @brief LiDAR切断
     * @details LiDARからの切断処理を行う
     */
    void disconnect(void);
    
    /**
     * @brief LiDAR再接続
     * @details LiDARからの切断処理および接続処理を行う
     */
    void reconnect(void);
    
    /**
     * @brief スキャンスレッド
     * @details LiDARからスキャンデータを受信しトピックとして配信を行う
     */
    void scan_thread(void);
    
    /**
     * @brief スキャントピック作成
     * @details LiDARから取得したスキャン情報のトピックへの変換を行う
     * @param[out] msg スキャンデータメッセージ
     * @retval true 正常終了
     * @retval false 取得失敗
     */
    bool create_scan_message(sensor_msgs::LaserScan & msg, long* distance, unsigned short* intensity);
    
    /**
     * @brief SafetyDataトピック作成
     * @details LiDARから取得したSafetyData情報のトピックへの変換を行う
     * @param[out] msg SafetyDataメッセージ
     * @retval true 正常終了
     * @retval false 取得失敗
     */
    bool create_safety_message(safety_data_message::SafetyData & msg, safety_data_t safety_data);
    
    /**
     * @brief スキャンスレッドの開始
     * @details LiDARとの通信を行うスキャンスレッドの開始を行う
     */
    void start_thread(void);
    
    /**
     * @brief スキャンスレッドの停止
     * @details LiDARとの通信を行うスキャンスレッドの停止および停止待機を行う
     */
    void stop_thread(void);

  public:

    /**
     * @brief コンストラクタ
     * @details クラス内メンバ変数の初期化、パラメータの宣言を行う
     */
    SafetyUrgNode();
    /**
     * @brief デストラクタ
     * @details スキャンスレッドの終了処理を行う
     */
    ~SafetyUrgNode();

};

#endif // SAFETY_URG_NODE

