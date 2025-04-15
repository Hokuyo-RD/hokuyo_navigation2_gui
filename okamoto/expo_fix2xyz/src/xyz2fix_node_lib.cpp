#define DEBUG_MODE
#include "expo_fix2xyz/xyz2fix_node_lib.h"



// odomコールバック関数.
void XYZLLATransNode::odom_callback1(const nav_msgs::Odometry &msg){
    debug_msg = "get odom_msg";
    DEBUG_PRINT(debug_msg);

    expo_fix_msgs::FixWithOrientation ret_msg;
    sensor_msgs::NavSatFix fix_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;
    std::string lio_frame = msg.header.frame_id;
    std::cout << "map_frame : " << map_frame << std::endl;
    std::cout << "tar_frame : " << lio_frame << std::endl;


    // フレーム変換.
    geometry_msgs::TransformStamped transformStamped;
    try{
        transformStamped = tfBuffer.lookupTransform(map_frame, lio_frame, ros::Time(0));
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

    fix_msg.header = msg.header ;
    fix_msg.header.frame_id = "" ;
    fix_msg.latitude = latlonalt.latitude;
    fix_msg.longitude = latlonalt.longitude;
    fix_msg.altitude = latlonalt.altitude;
    fix_msg.position_covariance[0] = msg.pose.covariance[0];
    fix_msg.position_covariance[1] = msg.pose.covariance[1];
    fix_msg.position_covariance[2] = msg.pose.covariance[2];
    fix_msg.position_covariance[3] = msg.pose.covariance[6];
    fix_msg.position_covariance[4] = msg.pose.covariance[7];
    fix_msg.position_covariance[5] = msg.pose.covariance[8];
    fix_msg.position_covariance[6] = msg.pose.covariance[12];
    fix_msg.position_covariance[7] = msg.pose.covariance[13];
    fix_msg.position_covariance[8] = msg.pose.covariance[14];

    ret_msg.fix = fix_msg;
    ret_msg.orientation = msg.pose.pose.orientation;

    fix_pub1.publish(ret_msg);

    debug_msg = "published fix_msg1 from odometry";
    DEBUG_PRINT(debug_msg);
}


void XYZLLATransNode::pose_callback2(const geometry_msgs::PoseStamped &msg){
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


void XYZLLATransNode::posecov_callback3(const geometry_msgs::PoseWithCovarianceStamped &msg){
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

void XYZLLATransNode::area_callback(const expo_msgs::Area &msg){
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

void XYZLLATransNode::person_callback(const expo_msgs::Person &msg){
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

void XYZLLATransNode::maigo_callback(const geometry_msgs::Point &msg){
    debug_msg = "get maigo_msg";
    DEBUG_PRINT(debug_msg);

    sensor_msgs::NavSatFix ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;


    // Vector3型はframe_idがないため、mapフレームを仮定する.
    // もし大学が別フレームで提供してきたら変更する.
    xyz(0) = msg.x ;
    xyz(1) = msg.y ;
    xyz(2) = msg.z ;

    latlonalt = l_u_transformer.get_latlonalt_from_xyz(xyz);

    ret_msg.latitude = latlonalt.latitude;
    ret_msg.longitude = latlonalt.longitude;
    ret_msg.altitude = latlonalt.altitude;

    maigo_fix_pub.publish(ret_msg);

    debug_msg = "published maigo_fix from maigo";
    DEBUG_PRINT(debug_msg);
}

void XYZLLATransNode::otosimono_callback(const geometry_msgs::Point &msg){
    debug_msg = "get otosimono_msg";
    DEBUG_PRINT(debug_msg);

    sensor_msgs::NavSatFix ret_msg;
    Eigen::Vector3d xyz;
    fix_xyz_trans::LatLonAlt latlonalt;


    // Vector3型はframe_idがないため、mapフレームを仮定する.
    // もし大学が別フレームで提供してきたら変更する.
    xyz(0) = msg.x ;
    xyz(1) = msg.y ;
    xyz(2) = msg.z ;

    latlonalt = l_u_transformer.get_latlonalt_from_xyz(xyz);

    ret_msg.latitude = latlonalt.latitude;
    ret_msg.longitude = latlonalt.longitude;
    ret_msg.altitude = latlonalt.altitude;

    otosimono_fix_pub.publish(ret_msg);

    debug_msg = "published otosimono_fix from otosimono";
    DEBUG_PRINT(debug_msg);
}



// 初期化処理.
XYZLLATransNode::XYZLLATransNode( ) : nh(), pnh("~"), tfListener(tfBuffer) {

    // 各rosparamのデフォルト値.
    sub_odom_topic1 = "odom1";
    sub_pose_topic2 = "pose2";
    sub_posecov_topic3 = "posecov3";
    sub_area_topic = "area";
    sub_person_topic = "person";
    sub_maigo_topic = "maigo";
    sub_otosimono_topic = "otosimono";

    pub_fix_topic1 = "fix/from_odom1";
    pub_fix_topic2 = "fix/from_pose2";
    pub_fix_topic3 = "fix/from_posecov3";
    pub_area_fix_topic = "area_fix/from_area";
    pub_person_fix_topic = "person_fix/from_person";
    pub_maigo_fix_topic = "fix/maigo";
    pub_otosimono_fix_topic = "fix/otosimono";

    map_frame = "map";
    
    fix_xyz_trans::LatLonAlt origin_latlonalt_;
    Eigen::Vector4d origin_quat_;
    
    
    // rosparamの取得.
    pnh.getParam("sub_odom_topic1", sub_odom_topic1);
    pnh.getParam("sub_pose_topic2", sub_pose_topic2);
    pnh.getParam("sub_posecov_topic3", sub_posecov_topic3);
    pnh.getParam("sub_area_topic", sub_area_topic);
    pnh.getParam("sub_person_topic", sub_person_topic);
    pnh.getParam("sub_maigo_topic", sub_maigo_topic);
    pnh.getParam("sub_otosimono_topic", sub_otosimono_topic);
    
    pnh.getParam("pub_fix_topic1", pub_fix_topic1);
    pnh.getParam("pub_fix_topic2", pub_fix_topic2);
    pnh.getParam("pub_fix_topic3", pub_fix_topic3);
    pnh.getParam("pub_area_fix_topic", pub_area_fix_topic);
    pnh.getParam("pub_person_fix_topic", pub_person_fix_topic);
    pnh.getParam("pub_maigo_fix_topic", pub_maigo_fix_topic);
    pnh.getParam("pub_otosimono_fix_topic", pub_otosimono_fix_topic);

    pnh.getParam("map_frame", map_frame);

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
    odom_sub1 = nh.subscribe(sub_odom_topic1, 10, &XYZLLATransNode::odom_callback1, this);
    pose_sub2 = nh.subscribe(sub_pose_topic2, 10, &XYZLLATransNode::pose_callback2, this);
    posecov_sub3 = nh.subscribe(sub_posecov_topic3, 10, &XYZLLATransNode::posecov_callback3, this);
    area_sub = nh.subscribe(sub_area_topic, 10, &XYZLLATransNode::area_callback, this);
    person_sub = nh.subscribe(sub_person_topic, 10, &XYZLLATransNode::person_callback, this);
    maigo_sub = nh.subscribe(sub_maigo_topic, 10, &XYZLLATransNode::maigo_callback, this);
    otosimono_sub = nh.subscribe(sub_otosimono_topic, 10, &XYZLLATransNode::otosimono_callback, this);

    fix_pub1 = nh.advertise<expo_fix_msgs::FixWithOrientation>(pub_fix_topic1, 10);
    fix_pub2 = nh.advertise<sensor_msgs::NavSatFix>(pub_fix_topic2, 10);
    fix_pub3 = nh.advertise<sensor_msgs::NavSatFix>(pub_fix_topic3, 10);
    area_fix_pub = nh.advertise<expo_fix_msgs::AreaFix>(pub_area_fix_topic, 10);
    person_fix_pub = nh.advertise<expo_fix_msgs::PersonFix>(pub_person_fix_topic, 10);
    maigo_fix_pub = nh.advertise<sensor_msgs::NavSatFix>(pub_maigo_fix_topic, 10);
    otosimono_fix_pub = nh.advertise<sensor_msgs::NavSatFix>(pub_otosimono_fix_topic, 10);

}

XYZLLATransNode::~XYZLLATransNode() {
}
