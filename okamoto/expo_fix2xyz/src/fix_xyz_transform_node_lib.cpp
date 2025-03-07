#define DEBUG_MODE
#include "expo_fix2xyz/fix_xyz_transform_node_lib.h"



// odomコールバック関数.
void LatlonUtmTransNode::odom_callback1(const nav_msgs::Odometry &msg){
    debug_msg = "get odom_msg";
    DEBUG_PRINT(debug_msg);

    sensor_msgs::NavSatFix ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;


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

    fix_pub1.publish(ret_msg);

    debug_msg = "published fix_msg1 from odometry";
    DEBUG_PRINT(debug_msg);
}


void LatlonUtmTransNode::pose_callback2(const geometry_msgs::PoseStamped &msg){
    debug_msg = "get odom_msg";
    DEBUG_PRINT(debug_msg);

    sensor_msgs::NavSatFix ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;


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
    tf2::doTransform(msg.pose, pose_on_map, transformStamped);

    xyz(0) = pose_on_map.position.x ;
    xyz(1) = pose_on_map.position.y ;
    xyz(2) = pose_on_map.position.z ;

    latlonalt = l_u_transformer.get_latlonalt_from_xyz(xyz);

    ret_msg.header = msg.header ;
    ret_msg.header.frame_id = "" ;
    ret_msg.latitude = latlonalt.latitude;
    ret_msg.longitude = latlonalt.longitude;
    ret_msg.altitude = latlonalt.altitude;

    fix_pub2.publish(ret_msg);

    debug_msg = "published fix_msg2 from pose_stamped";
    DEBUG_PRINT(debug_msg);
}


void LatlonUtmTransNode::posecov_callback3(const geometry_msgs::PoseWithCovarianceStamped &msg){
    debug_msg = "get odom_msg";
    DEBUG_PRINT(debug_msg);

    sensor_msgs::NavSatFix ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;


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

    fix_pub3.publish(ret_msg);

    debug_msg = "published fix_msg3 from pose_with_covariance_stamped";
    DEBUG_PRINT(debug_msg);
}

// areaコールバック
void LatlonUtmTransNode::area_callback(const expo_msgs::Area &msg){
    debug_msg = "get area_msg";
    DEBUG_PRINT(debug_msg);

    expo_fix_msgs::AreaFix ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;

    xyz(0) = msg.position.x ;
    xyz(1) = msg.position.y ;
    xyz(2) = msg.position.z ;

    latlonalt = l_u_transformer.get_latlonalt_from_xyz(xyz);

    ret_msg.id = msg.id;
    ret_msg.latitude = latlonalt.latitude;
    ret_msg.longitude = latlonalt.longitude;
    ret_msg.altitude = latlonalt.altitude;
    ret_msg.size = msg.size;
    ret_msg.velocity = msg.velocity;
    ret_msg.density = msg.density;
    ret_msg.pose = msg.pose;

    
    area_fix_pub.publish(ret_msg);

    debug_msg = "published area_fix_msg from area_msg";
    DEBUG_PRINT(debug_msg);
}


// personコールバック
void LatlonUtmTransNode::person_callback(const expo_msgs::Person &msg){
    debug_msg = "get person_msg";
    DEBUG_PRINT(debug_msg);

    expo_fix_msgs::PersonFix ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;

    xyz(0) = msg.position.x ;
    xyz(1) = msg.position.y ;
    xyz(2) = msg.position.z ;

    latlonalt = l_u_transformer.get_latlonalt_from_xyz(xyz);

    ret_msg.latitude = latlonalt.latitude;
    ret_msg.longitude = latlonalt.longitude;
    ret_msg.altitude = latlonalt.altitude;
    ret_msg.orientation = msg.orientation;
    ret_msg.velocity = msg.velocity;

    person_fix_pub.publish(ret_msg);

    debug_msg = "published person_fix_msg from person_msg";
    DEBUG_PRINT(debug_msg);
}

// fixコールバック関数.
void LatlonUtmTransNode::fix_callback1(const sensor_msgs::NavSatFix &msg){
    debug_msg = "get fix_msg1";
    DEBUG_PRINT(debug_msg);

    nav_msgs::Odometry ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;

    latlonalt.latitude = msg.latitude;
    latlonalt.longitude = msg.longitude;
    latlonalt.altitude = msg.altitude;

    xyz = l_u_transformer.get_xyz_from_latlonalt(latlonalt);

    ret_msg.header = msg.header;
    ret_msg.header.frame_id = map_frame;
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
    if(fix1_isfirst){
        fix1_isfirst = false;
    }
    else{
        // 移動量がある程度ある場合に角度推定.
        double movement_x = xyz(0) - last_fix1_pose.position.x;
        double movement_y = xyz(1) - last_fix1_pose.position.y;
        double movement = movement_x * movement_x + movement_y * movement_y;
        if(movement > 0.1){
            double theta = std::atan2(movement_y , movement_x);
            ret_msg.pose.pose.orientation.z = std::sin(0.5*theta);
            ret_msg.pose.pose.orientation.w = std::cos(0.5*theta);
        }
    }
    last_fix1_pose = ret_msg.pose.pose;

    odom_pub1.publish(ret_msg);
    
    debug_msg = "published odometry_msg from fix_msg1";
    DEBUG_PRINT(debug_msg);
}



void LatlonUtmTransNode::fix_callback2(const sensor_msgs::NavSatFix &msg){
    debug_msg = "get fix_msg2";
    DEBUG_PRINT(debug_msg);

    geometry_msgs::PoseStamped ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;

    latlonalt.latitude = msg.latitude;
    latlonalt.longitude = msg.longitude;
    latlonalt.altitude = msg.altitude;

    xyz = l_u_transformer.get_xyz_from_latlonalt(latlonalt);

    ret_msg.header = msg.header;
    ret_msg.header.frame_id = map_frame;
    ret_msg.pose.position.x = xyz(0);
    ret_msg.pose.position.y = xyz(1);
    ret_msg.pose.position.z = xyz(2);
    ret_msg.pose.orientation.w = 1.0;

    // 姿勢推定.
    if(fix2_isfirst){
        fix2_isfirst = false;
    }
    else{
        // 移動量がある程度ある場合に角度推定.
        double movement_x = xyz(0) - last_fix2_pose.position.x;
        double movement_y = xyz(1) - last_fix2_pose.position.y;
        double movement = movement_x * movement_x + movement_y * movement_y;
        if(movement > 0.1){
            double theta = std::atan2(movement_y , movement_x);
            ret_msg.pose.orientation.z = std::sin(0.5*theta);
            ret_msg.pose.orientation.w = std::cos(0.5*theta);
        }
    }
    last_fix2_pose = ret_msg.pose;

    pose_pub2.publish(ret_msg);
    
    debug_msg = "published pose_stamped_msg from fix_msg2";
    DEBUG_PRINT(debug_msg);
}


void LatlonUtmTransNode::fix_callback3(const sensor_msgs::NavSatFix &msg){
    debug_msg = "get fix_msg3";
    DEBUG_PRINT(debug_msg);

    geometry_msgs::PoseWithCovarianceStamped ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;

    latlonalt.latitude = msg.latitude;
    latlonalt.longitude = msg.longitude;
    latlonalt.altitude = msg.altitude;

    xyz = l_u_transformer.get_xyz_from_latlonalt(latlonalt);

    ret_msg.header = msg.header;
    ret_msg.header.frame_id = map_frame;
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
    if(fix3_isfirst){
        fix3_isfirst = false;
    }
    else{
        // 移動量がある程度ある場合に角度推定.
        double movement_x = xyz(0) - last_fix3_pose.position.x;
        double movement_y = xyz(1) - last_fix3_pose.position.y;
        double movement = movement_x * movement_x + movement_y * movement_y;
        if(movement > 0.1){
            double theta = std::atan2(movement_y , movement_x);
            ret_msg.pose.pose.orientation.z = std::sin(0.5*theta);
            ret_msg.pose.pose.orientation.w = std::cos(0.5*theta);
        }
    }
    last_fix3_pose = ret_msg.pose.pose;

    posecov_pub3.publish(ret_msg);
    
    debug_msg = "published pose_with_covariance_stamped_msg from fix_msg3";
    DEBUG_PRINT(debug_msg);
}


void LatlonUtmTransNode::area_fix_callback(const expo_fix_msgs::AreaFix &msg){
    debug_msg = "get area_fix_msg";
    DEBUG_PRINT(debug_msg);

    expo_msgs::Area ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;

    latlonalt.latitude = msg.latitude;
    latlonalt.longitude = msg.longitude;
    latlonalt.altitude = msg.altitude;

    xyz = l_u_transformer.get_xyz_from_latlonalt(latlonalt);

    ret_msg.id = msg.id;
    ret_msg.position.x = xyz(0);
    ret_msg.position.y = xyz(1);
    ret_msg.position.z = xyz(2);
    ret_msg.size = msg.size;
    ret_msg.velocity = msg.velocity;
    ret_msg.density = msg.density;
    ret_msg.pose = msg.pose;

    area_pub.publish(ret_msg);
    
    debug_msg = "published area_msg from area_fix_msg";
    DEBUG_PRINT(debug_msg);
}

void LatlonUtmTransNode::person_fix_callback(const expo_fix_msgs::PersonFix &msg){
    debug_msg = "get person_fix_msg";
    DEBUG_PRINT(debug_msg);

    expo_msgs::Person ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;

    latlonalt.latitude = msg.latitude;
    latlonalt.longitude = msg.longitude;
    latlonalt.altitude = msg.altitude;

    xyz = l_u_transformer.get_xyz_from_latlonalt(latlonalt);

    ret_msg.position.x = xyz(0);
    ret_msg.position.y = xyz(1);
    ret_msg.position.z = xyz(2);
    ret_msg.orientation = msg.orientation;
    ret_msg.velocity = msg.velocity;
    
    person_pub.publish(ret_msg);
    
    debug_msg = "published person_msg from person_fix_msg";
    DEBUG_PRINT(debug_msg);
}




// 初期化処理.
LatlonUtmTransNode::LatlonUtmTransNode( ) : nh(), pnh("~") {

    tf2_ros::TransformListener tfListener(tfBuffer);

    // 各rosparamのデフォルト値.
    sub_fix_topic1 = "fix1";
    sub_fix_topic2 = "fix2";
    sub_fix_topic3 = "fix3";
    sub_odom_topic1 = "odom1";
    sub_pose_topic2 = "pose2";
    sub_posecov_topic3 = "posecov3";
    sub_area_topic = "area";
    sub_area_fix_topic = "area_fix";
    sub_person_topic = "person";
    sub_person_fix_topic = "person_fix";

    pub_fix_topic1 = "fix/from_odom1";
    pub_fix_topic2 = "fix/from_pose2";
    pub_fix_topic3 = "fix/from_posecov3";
    pub_odom_topic1 = "odometry/from_fix1";
    pub_pose_topic2 = "pose/from_fix2";
    pub_posecov_topic3 = "posecov/from_fix3";
    pub_area_topic = "area/from_fix";
    pub_area_fix_topic = "area_fix/from_area";
    pub_person_topic = "person/from_person_fix";
    pub_person_fix_topic = "person_fix/from_person";

    map_frame = "map";
    rot_cov = 1000000000.0;
    make_angle_from_movement = false;
    
    fix_xyz_trans::LatLonAlt origin_latlonalt_;
    Eigen::Vector4d origin_quat_;
    
    
    // rosparamの取得.
    pnh.getParam("sub_fix_topic1", sub_fix_topic1);
    pnh.getParam("sub_fix_topic2", sub_fix_topic2);
    pnh.getParam("sub_fix_topic3", sub_fix_topic3);
    pnh.getParam("sub_odom_topic1", sub_odom_topic1);
    pnh.getParam("sub_pose_topic2", sub_pose_topic2);
    pnh.getParam("sub_posecov_topic3", sub_posecov_topic3);

    pnh.getParam("sub_area_topic", sub_area_topic);
    pnh.getParam("sub_area_fix_topic", sub_area_fix_topic);
    pnh.getParam("sub_person_topic", sub_person_topic);
    pnh.getParam("sub_person_fix_topic", sub_person_fix_topic);
    
    pnh.getParam("pub_fix_topic1", pub_fix_topic1);
    pnh.getParam("pub_fix_topic2", pub_fix_topic2);
    pnh.getParam("pub_fix_topic3", pub_fix_topic3);
    pnh.getParam("pub_oodm_topic1", pub_odom_topic1);
    pnh.getParam("pub_pose_topic2", pub_pose_topic2);
    pnh.getParam("pub_posecov_topic3", pub_posecov_topic3);
    pnh.getParam("rot_cov", rot_cov);
    pnh.getParam("make_angle_from_movement", make_angle_from_movement);
    pnh.getParam("map_frame", map_frame);

    pnh.getParam("pub_area_topic", pub_area_topic);
    pnh.getParam("pub_area_fix_topic", pub_area_fix_topic);
    pnh.getParam("pub_person_topic", pub_person_topic);
    pnh.getParam("pub_person_fix_topic", pub_person_fix_topic);
    
    if (pnh.getParam("epsg_code_num", epsg_code_num)){
        l_u_transformer.set_epsg_code(epsg_code_num);
    }

    if (pnh.getParam("origin_quat", origin_quat_str) && pnh.getParam("origin_pose", origin_pose_str)){
        DEBUG_PRINT(origin_pose_str);
        DEBUG_PRINT(origin_quat_str);
        fix_xyz_trans::LatLonAlt latlonalt;
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
    fix_sub1 = nh.subscribe(sub_fix_topic1, 10, &LatlonUtmTransNode::fix_callback1, this);
    fix_sub2 = nh.subscribe(sub_fix_topic2, 10, &LatlonUtmTransNode::fix_callback2, this);
    fix_sub3 = nh.subscribe(sub_fix_topic3, 10, &LatlonUtmTransNode::fix_callback3, this);
    odom_sub1 = nh.subscribe(sub_odom_topic1, 10, &LatlonUtmTransNode::odom_callback1, this);
    pose_sub2 = nh.subscribe(sub_pose_topic2, 10, &LatlonUtmTransNode::pose_callback2, this);
    posecov_sub3 = nh.subscribe(sub_posecov_topic3, 10, &LatlonUtmTransNode::posecov_callback3, this);

    area_sub = nh.subscribe(sub_area_topic, 10, &LatlonUtmTransNode::area_callback, this);
    area_fix_sub = nh.subscribe(sub_area_fix_topic, 10, &LatlonUtmTransNode::area_fix_callback, this);
    person_sub = nh.subscribe(sub_person_topic, 10, &LatlonUtmTransNode::person_callback, this);
    person_fix_sub = nh.subscribe(sub_person_fix_topic, 10, &LatlonUtmTransNode::person_fix_callback, this);

    fix_pub1 = nh.advertise<sensor_msgs::NavSatFix>(pub_fix_topic1, 10);
    fix_pub2 = nh.advertise<sensor_msgs::NavSatFix>(pub_fix_topic2, 10);
    fix_pub3 = nh.advertise<sensor_msgs::NavSatFix>(pub_fix_topic3, 10);
    odom_pub1 = nh.advertise<nav_msgs::Odometry>(pub_odom_topic1, 10);
    pose_pub2 = nh.advertise<geometry_msgs::PoseStamped>(pub_pose_topic2, 10);
    posecov_pub3 = nh.advertise<geometry_msgs::PoseWithCovarianceStamped>(pub_posecov_topic3, 10);

    area_pub = nh.advertise<expo_msgs::Area>(pub_area_topic, 10);
    area_fix_pub = nh.advertise<expo_fix_msgs::AreaFix>(pub_area_fix_topic, 10);
    person_pub = nh.advertise<expo_msgs::Person>(pub_person_topic, 10);
    person_fix_pub = nh.advertise<expo_fix_msgs::PersonFix>(pub_person_fix_topic, 10);

    fix1_isfirst = true;
    fix2_isfirst = true;
    fix3_isfirst = true;

}

LatlonUtmTransNode::~LatlonUtmTransNode() {
}
