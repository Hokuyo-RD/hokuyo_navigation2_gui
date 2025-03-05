#define DEBUG_MODE

#include "expo_latlon2xyz/latlon_utm_transform_node_lib.h"

// utmコールバック関数.
void LatlonUtmTransNode::utm_callback1(const nav_msgs::Odometry &msg){
    debug_msg = "get utm_msg";
    DEBUG_PRINT(debug_msg);

    sensor_msgs::NavSatFix ret_msg;
    Eigen::Vector3d xyz;
    latlon_utm_trans::LatLonAlt latlonalt;


    // フレーム変換.
    geometry_msgs::TransformStamped transformStamped;
    try{
        transformStamped = tfBuffer.lookupTransform(map_frame, msg.header.frame_id, ros::Time(0));
    }
    catch (tf2::TransformException& ex){
        ROS_WARN("%s", ex.what());
        return;
    }
    geometry_msgs::Pose pose_on_map;
    tf2::doTransform(msg.pose.pose, pose_on_map, transformStamped);

    xyz(0) = pose_on_map.position.x ;
    xyz(1) = pose_on_map.position.y ;
    xyz(2) = pose_on_map.position.z ;

    latlonalt = l_u_transformer.get_latlonalt_from_xyz(xyz);

    ret_msg.header = msg.header ;
    ret_msg.header.frame_id = "" ;
    ret_msg.latitude = latlonalt.latitude;
    ret_msg.longitude = latlonalt.longitude;
    ret_msg.altitude = latlonalt.altitude;
    ret_msg.position_covariance[0] = msg.pose.covariance[0];
    ret_msg.position_covariance[1] = msg.pose.covariance[1];
    ret_msg.position_covariance[2] = msg.pose.covariance[2];
    ret_msg.position_covariance[3] = msg.pose.covariance[6];
    ret_msg.position_covariance[4] = msg.pose.covariance[7];
    ret_msg.position_covariance[5] = msg.pose.covariance[8];
    ret_msg.position_covariance[6] = msg.pose.covariance[12];
    ret_msg.position_covariance[7] = msg.pose.covariance[13];
    ret_msg.position_covariance[8] = msg.pose.covariance[14];

    latlon_pub1.publish(ret_msg);

    debug_msg = "published latlon_msg from xyz";
    DEBUG_PRINT(debug_msg);
}

// latlonコールバック関数.
void LatlonUtmTransNode::latlon_callback1(const sensor_msgs::NavSatFix &msg){
    debug_msg = "get latlon_msg";
    DEBUG_PRINT(debug_msg);

    nav_msgs::Odometry ret_msg;
    Eigen::Vector3d xyz;
    latlon_utm_trans::LatLonAlt latlonalt;

    latlonalt.latitude = msg.latitude;
    latlonalt.longitude = msg.longitude;
    latlonalt.altitude = msg.altitude;

    xyz = l_u_transformer.get_xyz_from_latlonalt(latlonalt);

    ret_msg.header = msg.header;
    ret_msg.pose.pose.position.x = xyz(0);
    ret_msg.pose.pose.position.y = xyz(1);
    ret_msg.pose.pose.position.z = xyz(2);
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
        double movement_x = xyz(0) - last_latlon_pose.position.x;
        double movement_y = xyz(1) - last_latlon_pose.position.y;
        double movement = movement_x * movement_x + movement_y * movement_y;
        if(movement > 0.1){
            double theta = std::atan2(movement_y , movement_x);
            ret_msg.pose.pose.orientation.z = std::sin(0.5*theta);
            ret_msg.pose.pose.orientation.w = std::cos(0.5*theta);
        }
    }
    last_latlon_pose = ret_msg.pose.pose;

    utm_pub1.publish(ret_msg);
    
    debug_msg = "published utm_msg from latlon";
    DEBUG_PRINT(debug_msg);
}

// 初期化処理.
LatlonUtmTransNode::LatlonUtmTransNode( ) : nh(), pnh("~") {

    tf2_ros::TransformListener tfListener(tfBuffer);

    // 各rosparamのデフォルト値.
    sub_latlon_topic1 = "fix1";
    sub_latlon_topic2 = "fix2";
    sub_latlon_topic3 = "fix3";
    sub_utm_topic1 = "odometry/utm1";
    sub_utm_topic2 = "odometry/utm2";
    sub_utm_topic3 = "odometry/utm3";
    pub_latlon_topic1 = "fix/utm1";
    pub_latlon_topic2 = "fix/utm2";
    pub_latlon_topic3 = "fix/utm3";
    pub_utm_topic1 = "odometry/fix1";
    pub_utm_topic2 = "odometry/fix2";
    pub_utm_topic3 = "odometry/fix3";
    map_frame = "map";
    rot_cov = 1000000000.0;
    make_angle_from_movement = false;
    
    latlon_utm_trans::LatLonAlt origin_latlonalt_;
    Eigen::Vector4d origin_quat_;
    
    
    // rosparamの取得.
    pnh.getParam("sub_latlon_topic1", sub_latlon_topic1);
    pnh.getParam("sub_latlon_topic2", sub_latlon_topic2);
    pnh.getParam("sub_latlon_topic3", sub_latlon_topic3);
    pnh.getParam("sub_utm_topic1", sub_utm_topic1);
    pnh.getParam("sub_utm_topic2", sub_utm_topic2);
    pnh.getParam("sub_utm_topic3", sub_utm_topic3);
    pnh.getParam("pub_latlon_topic1", pub_latlon_topic1);
    pnh.getParam("pub_latlon_topic2", pub_latlon_topic2);
    pnh.getParam("pub_latlon_topic3", pub_latlon_topic3);
    pnh.getParam("pub_utm_topic1", pub_utm_topic1);
    pnh.getParam("pub_utm_topic2", pub_utm_topic2);
    pnh.getParam("pub_utm_topic3", pub_utm_topic3);
    pnh.getParam("rot_cov", rot_cov);
    pnh.getParam("make_angle_from_movement", make_angle_from_movement);
    pnh.getParam("map_frame", map_frame);
    
    if (pnh.getParam("epsg_code_num", epsg_code_num)){
        l_u_transformer.set_epsg_code(epsg_code_num);
    }

    if (pnh.getParam("origin_quat", origin_quat_str) && pnh.getParam("origin_pose", origin_pose_str)){
        DEBUG_PRINT(origin_pose_str);
        DEBUG_PRINT(origin_quat_str);
        latlon_utm_trans::LatLonAlt latlonalt;
        double qx = 0.0, qy = 0.0, qz = 0.0, qw = 1.0;
        
        std::replace(origin_pose_str.begin(), origin_pose_str.end(), ',', ' ');
        std::istringstream iss_pose(origin_pose_str);
        iss_pose >> latlonalt.latitude >> latlonalt.longitude >> latlonalt.altitude;

        std::replace(origin_quat_str.begin(), origin_quat_str.end(), ',', ' ');
        std::istringstream iss_quat(origin_quat_str);
        iss_quat >> qx >> qy >> qz >> qw;
        
        origin_quat_ << qx, qy, qz, qw;
        if (origin_quat_.norm() == 0){
            std::cerr << "invalid quaternion" << std::endl;
            origin_quat_ << 0.0 , 0.0 , 0.0 , 1.0;
        }
        else{
            origin_quat_.normalize();
        }
        DEBUG_PRINT(latlonalt.latitude);
        DEBUG_PRINT(latlonalt.longitude);
        DEBUG_PRINT(latlonalt.altitude);
        DEBUG_PRINT(origin_quat_);

        l_u_transformer.set_origin(latlonalt, origin_quat_);
    }

    // publisher,subscriberの設定.
    latlon_sub1 = nh.subscribe(sub_latlon_topic1, 10, &LatlonUtmTransNode::latlon_callback1, this);
    //latlon_sub2 = nh.subscribe(sub_latlon_topic2, 10, &LatlonUtmTransNode::latlon_callback2, this);
    //latlon_sub3 = nh.subscribe(sub_latlon_topic3, 10, &LatlonUtmTransNode::latlon_callback3, this);
    utm_sub1 = nh.subscribe(sub_utm_topic1, 10, &LatlonUtmTransNode::utm_callback1, this);
    //utm_sub2 = nh.subscribe(sub_utm_topic2, 10, &LatlonUtmTransNode::utm_callback2, this);
    //utm_sub3 = nh.subscribe(sub_utm_topic3, 10, &LatlonUtmTransNode::utm_callback3, this);
    latlon_pub1 = nh.advertise<sensor_msgs::NavSatFix>(pub_latlon_topic1, 10);
    //latlon_pub2 = nh.advertise<sensor_msgs::NavSatFix>(pub_latlon_topic2, 10);
    //latlon_pub3 = nh.advertise<sensor_msgs::NavSatFix>(pub_latlon_topic3, 10);
    utm_pub1 = nh.advertise<nav_msgs::Odometry>(pub_utm_topic1, 10);
    //utm_pub2 = nh.advertise<nav_msgs::Odometry>(pub_utm_topic2, 10);
    //utm_pub3 = nh.advertise<nav_msgs::Odometry>(pub_utm_topic3, 10);

    latlon_isfirst = true;

}

LatlonUtmTransNode::~LatlonUtmTransNode() {
}
