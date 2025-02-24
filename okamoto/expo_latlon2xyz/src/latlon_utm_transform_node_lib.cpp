#define DEBUG_MODE
#include <coordinate_transform/latlon_utm_transform_node_lib.h>


// utmコールバック関数.
void LatlonUtmTransNode::utm_callback(const nav_msgs::Odometry &msg){
    debug_msg = "get utm_msg";
    DEBUG_PRINT(debug_msg);

    sensor_msgs::NavSatFix ret_msg;
    latlon_utm_trans::UTM utm;
    latlon_utm_trans::LatLon latlon;


    utm.x = msg.pose.pose.position.x ;
    utm.y = msg.pose.pose.position.y ;
    std::string id_str = msg.header.frame_id;
    DEBUG_PRINT(id_str);
    try {
        std::string utm_zone_str = id_str.substr(8, 2);
        utm.zone = std::stoi(utm_zone_str);
    }
    catch (const std::invalid_argument& e) {
        std::cout << "invalid frame_id(utm_zone)" << std::endl;
        return;
    }

    latlon = l_u_transformer.get_latlon_from_utm(utm);

    ret_msg.header = msg.header ;
    ret_msg.header.frame_id = "" ;
    ret_msg.latitude = latlon.latitude;
    ret_msg.longitude = latlon.longitude;
    ret_msg.altitude = msg.pose.pose.position.z;
    ret_msg.position_covariance[0] = msg.pose.covariance[0];
    ret_msg.position_covariance[1] = msg.pose.covariance[1];
    ret_msg.position_covariance[2] = msg.pose.covariance[2];
    ret_msg.position_covariance[3] = msg.pose.covariance[6];
    ret_msg.position_covariance[4] = msg.pose.covariance[7];
    ret_msg.position_covariance[5] = msg.pose.covariance[8];
    ret_msg.position_covariance[6] = msg.pose.covariance[12];
    ret_msg.position_covariance[7] = msg.pose.covariance[13];
    ret_msg.position_covariance[8] = msg.pose.covariance[14];

    latlon_pub.publish(ret_msg);

    debug_msg = "published latlon_msg from utm";
    DEBUG_PRINT(debug_msg);
}

// latlonコールバック関数.
void LatlonUtmTransNode::latlon_callback(const sensor_msgs::NavSatFix &msg){
    debug_msg = "get latlon_msg";
    DEBUG_PRINT(debug_msg);

    nav_msgs::Odometry ret_msg;
    latlon_utm_trans::UTM utm;
    latlon_utm_trans::LatLon latlon;

    latlon.latitude = msg.latitude;
    latlon.longitude = msg.longitude;

    utm = l_u_transformer.get_utm_from_latlon(latlon);

    ret_msg.header = msg.header;
    ret_msg.header.frame_id = "utm/utm_" + std::to_string(utm.zone) + "Z";
    ret_msg.pose.pose.position.x = utm.x;
    ret_msg.pose.pose.position.y = utm.y;
    ret_msg.pose.pose.position.z = msg.altitude;
    ret_msg.pose.pose.orientation.w = 1.0;
    ret_msg.pose.covariance[0] = msg.position_covariance[0];
    ret_msg.pose.covariance[1] = msg.position_covariance[1];
    ret_msg.pose.covariance[2] = msg.position_covariance[2];
    ret_msg.pose.covariance[6] = msg.position_covariance[3];
    ret_msg.pose.covariance[7] = msg.position_covariance[4];
    ret_msg.pose.covariance[8] = msg.position_covariance[5];
    ret_msg.pose.covariance[12] = msg.position_covariance[6];
    ret_msg.pose.covariance[13] = msg.position_covariance[7];
    ret_msg.pose.covariance[14] = msg.position_covariance[8];
    ret_msg.pose.covariance[21] = rot_cov;
    ret_msg.pose.covariance[28] = rot_cov;
    ret_msg.pose.covariance[35] = rot_cov;

    // 姿勢推定.
    if(latlon_isfirst){
        latlon_isfirst = false;
    }
    else{
        // 移動量がある程度ある場合に角度推定.
        double movement_x = utm.x - last_latlon_pose.position.x;
        double movement_y = utm.y - last_latlon_pose.position.y;
        double movement = movement_x * movement_x + movement_y * movement_y;
        if(movement > 0.1){
            double theta = std::atan2(movement_y , movement_x);
            ret_msg.pose.pose.orientation.z = std::sin(0.5*theta);
            ret_msg.pose.pose.orientation.w = std::cos(0.5*theta);
        }
    }
    last_latlon_pose = ret_msg.pose.pose;

    utm_pub.publish(ret_msg);
    
    debug_msg = "published utm_msg from latlon";
    DEBUG_PRINT(debug_msg);
}

// 初期化処理.
LatlonUtmTransNode::LatlonUtmTransNode() : nh(), pnh("~") {

    // 各rosparamのデフォルト値.
    sub_latlon_topic = "navsatfix/fix";
    sub_utm_topic = "odometry/utm";
    pub_latlon_topic = "navsatfix/utm";
    pub_utm_topic = "odometry/fix";
    rot_cov = 1000000000.0;
    make_angle_from_movement = false;

    // rosparamの取得.
    pnh.getParam("sub_latlon_topic", sub_latlon_topic);
    pnh.getParam("sub_utm_topic", sub_utm_topic);
    pnh.getParam("pub_latlon_topic", pub_latlon_topic);
    pnh.getParam("pub_utm_topic", pub_utm_topic);
    pnh.getParam("rot_cov", rot_cov);
    pnh.getParam("make_angle_from_movement", make_angle_from_movement);

    // publisher,subscriberの設定.
    latlon_sub = nh.subscribe(sub_latlon_topic, 10, &LatlonUtmTransNode::latlon_callback, this);
    utm_sub = nh.subscribe(sub_utm_topic, 10, &LatlonUtmTransNode::utm_callback, this);
    latlon_pub = nh.advertise<sensor_msgs::NavSatFix>(pub_latlon_topic, 10);
    utm_pub = nh.advertise<nav_msgs::Odometry>(pub_utm_topic, 10);

    latlon_isfirst = true;

}

LatlonUtmTransNode::~LatlonUtmTransNode() {
}
