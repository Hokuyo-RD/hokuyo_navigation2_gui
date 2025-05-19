#define DEBUG_MODE
#include "safety_urg_node/safety_urg_node_lib.h"


// コンストラクタ
SafetyUrgNode::SafetyUrgNode( ) : nh(), pnh("~")
{
  // 初期化
  initialize();

  // スレッド起動
  start_thread();
}

// デストラクタ
SafetyUrgNode::~SafetyUrgNode() 
{
  // スレッドの停止
  stop_thread();
}

// 初期化
void SafetyUrgNode::initialize()
{
  // パラメータの登録
  pnh.param("scan_topic", scan_topic_, std::string("scan"));
  pnh.param("safety_topic", safety_topic_, std::string("safety_data"));
  pnh.param("ip_address", ip_address_, std::string("192.168.0.10"));
  pnh.param("ip_port", ip_port_, 10940);
  pnh.param("frame_id", frame_id_, std::string("laser"));
  pnh.param("calibrate_time", calibrate_time_, false);
  pnh.param("synchronize_time", synchronize_time_, false);
  pnh.param("error_limit", error_limit_, 4);
  pnh.param("error_reset_period", error_reset_period_, 10.0);
  pnh.param("continuous_mode", continuous_mode_, false);

  // 内部変数初期化
  error_count_ = 0;
  total_error_count_ = 0;
  reconnect_count_ = 0;
  is_connected_ = false;
  is_measurement_started_ = false;

  // メッセージヘッダのframe_id設定
  header_frame_id_ =
    (frame_id_.find_first_not_of('/') == std::string::npos) ? "" : frame_id_.substr(
    frame_id_.find_first_not_of('/'));

  // pub sub 設定
  scan_pub_ = nh.advertise<sensor_msgs::LaserScan>(scan_topic_, 10);
  safety_pub_ = nh.advertise<safety_data_message::SafetyData>(safety_topic_, 10);
}

// スキャン設定
void SafetyUrgNode::set_scan_parameter()
{
  // 現状は固定値を入れているため何もしない
  // 
}

// Lidarとの接続処理
bool SafetyUrgNode::connect()
{
  int argc = 2;
  char *argv[] = { const_cast<char*>(" "), const_cast<char*>(ip_address_.c_str()) };

  // イーサネット接続
  int ret = safety_sensor_open_safetyCMD(&sensor_, argc, argv);
  if(ret < 0){
    ROS_INFO("open error.");
    return false;
  }
  
  ROS_INFO("open success. IP_Address: %s", ip_address_.c_str());
  ROS_INFO("Sensor serial ID : %s", safety_sensor_serial_id_safetyCMD(&sensor_));

  is_connected_ = true;

  return true;
}

// Lidarとの切断処理
void SafetyUrgNode::disconnect()
{
  if (is_connected_) {
    if(continuous_mode_ == true){
      safety_sensor_stop_measurement_distance_intensity_safetyCMD(&sensor_);
    }

    safety_sensor_close_safetyCMD(&sensor_);
    is_connected_ = false;
  }
}

// Lidarとの再接続処理
void SafetyUrgNode::reconnect()
{
  // 切断
  disconnect();
  // 接続
  connect();
}

// scanスレッド
void SafetyUrgNode::scan_thread()
{
  int ret = 0;

  reconnect_count_ = 0;
  
  while (!close_thread_) {
    if (!is_connected_) {
      if (!connect()) {
        ros::Duration(0.5).sleep();
        continue;
      }
    }

    // スキャン設定
    set_scan_parameter();

    // 計測開始
    if(continuous_mode_ == true){
      ret = safety_sensor_request_distance_intensity_continuous_safetyCMD(&sensor_);
      if (ret < 0) {
        ROS_INFO("Could not start Hokuyo measurement\n");
        disconnect();
        
        reconnect();
        reconnect_count_++;
        
        continue;
      }
      
      ROS_INFO("get data Continuous mode.");
    }
    else{
      ROS_INFO("get data Handshake mode.");
    }

    is_measurement_started_ = true;
    error_count_ = 0;

    long distance[MAX_STEP_SIZE];
    unsigned short intensity[MAX_STEP_SIZE];
    safety_data_t safety_data;
    
    ret = 0;
    
    ros::Time prev_time = ros::Time::now();
    
    while (!close_thread_) {
      
      if(continuous_mode_ == false){
        ret = safety_sensor_request_distance_intensity_handshaking_safetyCMD(&sensor_);
        
        if(ret < 0){
          ROS_INFO("request command error.\n");
          error_count_++;
        }
      }
      
      // AR04 reception
      ret = safety_sensor_get_distance_intensity_safetyCMD(&sensor_, distance, intensity, &safety_data);
      if (ret < 0){
        ROS_INFO("Communication error or invalid response.\n");
        error_count_++;
      }
      else if (ret == 0){
        // Data is not available in the command
      }
      else if (ret < 254){
        ROS_INFO("Sensor status error: %x.\n", ret);
      }
      else{
        // スキャンデータ取得
        auto scan_msg_ = std::make_shared<sensor_msgs::LaserScan>();
        if (create_scan_message(*scan_msg_, distance, intensity)) {
          scan_pub_.publish(*scan_msg_);
        }
        else{
          ROS_INFO("Could not get scan data.");
          error_count_++;
          total_error_count_++;
        }
        
        // safetydata取得
        auto safety_msg_ = std::make_shared<safety_data_message::SafetyData>();
        if(create_safety_message(*safety_msg_, safety_data)) {
          safety_pub_.publish(*safety_msg_);  
        }
        else{
          ROS_INFO("Could not get safety data.");
          error_count_++;
          total_error_count_++;
        }
      }
      
      // エラーカウント判定
      if(error_count_ > error_limit_){
        ROS_INFO("Error count exceeded limit, reconnecting.");
        // 再接続処理
        is_measurement_started_ = false;
        reconnect();
        reconnect_count_++;
        break;
      }
      else{
        // エラーカウントのリセット
        ros::Time current_time = ros::Time::now();
        ros::Duration period = current_time - prev_time;
        if (period.toSec() >= error_reset_period_) {
          prev_time = current_time;
          error_count_ = 0;
        }
      }
    }
    
  }
  
  // 切断処理
  disconnect();
}

// スキャンメッセージ作成
bool SafetyUrgNode::create_scan_message(sensor_msgs::LaserScan & msg, long* distance, unsigned short* intensity)
{
  ros::Time system_time_stamp = ros::Time::now();

  // header
  msg.header.frame_id = header_frame_id_;
  msg.header.stamp = system_time_stamp;

  // 対応機種が1つのためパラメータに固有値を設定
  msg.angle_min = -2.356194496154785;
  msg.angle_max = 2.356194496154785;
  msg.angle_increment = 0.004363323096185923;
  msg.time_increment = 2.0833333110203966e-05;
  msg.scan_time = 0.029999999329447746;
  msg.range_min = 0.0;
  msg.range_max = 40.0;

  // データ領域確保
  msg.ranges.resize(MAX_STEP_SIZE);
  msg.intensities.resize(MAX_STEP_SIZE);

    // 距離・受光強度データ格納
  for(int i = 0; i < MAX_STEP_SIZE; i++){
    if(distance[i] != 0) {
      msg.ranges[i] = static_cast<float>(distance[i]) / 1000.0;
      msg.intensities[i] = intensity[i];
    }
    else{
      msg.ranges[i] = std::numeric_limits<float>::quiet_NaN();
      continue;
    }
  }

  return true;
}

// SafetyDataメッセージ作成
bool SafetyUrgNode::create_safety_message(safety_data_message::SafetyData & msg, safety_data_t safety_data)
{
  ros::Time system_time_stamp = ros::Time::now();

  // header
  msg.header.frame_id = header_frame_id_;
  msg.header.stamp = system_time_stamp;

  // SafetyData格納
  msg.operating_mode = safety_data.is_setting;
  msg.area_number = safety_data.area_number + 1;
  msg.timestamp = safety_data.timestamp;
  msg.error_status = safety_data.is_error_detected;
  msg.last_error_number = safety_data.error_code;
  msg.lockout_status = safety_data.is_lockout;
  msg.ossd_1_status = safety_data.is_ossd1_on;
  msg.ossd_2_status = safety_data.is_ossd2_on;
  msg.warning_1_status = safety_data.is_warning1_on;
  msg.warning_2_status = safety_data.is_warning2_on;
  msg.ossd_3_status = safety_data.is_ossd3_on;
  msg.ossd_4_status = safety_data.is_ossd4_on;
  msg.muting_override_1 = safety_data.is_mut_over1_on;
  msg.muting_override_2 = safety_data.is_mut_over2_on;
  msg.reset_request_1 = safety_data.is_reset_req1_on;
  msg.reset_request_2 = safety_data.is_reset_req2_on;
  msg.encoder_linear_velocity = static_cast<int32_t>(static_cast<int16_t>(safety_data.encoder_velocity));
  msg.laser_off_status = safety_data.is_laser_off;
  msg.contamination_warning = safety_data.is_optical_window_contamination_warning;
  msg.encoder_input_pattern_number = safety_data.encoder_input_pattern_num;
  msg.encoder_anglular_velocity = static_cast<int32_t>(static_cast<int16_t>(safety_data.encoder_angular_velocity));

  return true;
}

// スキャンスレッドの開始
void SafetyUrgNode::start_thread(void)
{
  // スレッド終了フラグのクリア
  close_thread_ = false;
  scan_thread_ = std::thread(std::bind(&SafetyUrgNode::scan_thread, this));
}

// スキャンスレッドの停止
void SafetyUrgNode::stop_thread(void)
{
  // スレッド終了フラグのセット
  close_thread_ = true;
  if (scan_thread_.joinable()) {
    scan_thread_.join();
  }
}

