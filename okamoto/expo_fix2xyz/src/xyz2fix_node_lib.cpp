#define DEBUG_MODE
#include "expo_fix2xyz/xyz2fix_node_lib.h"



// 落とし物のコールバック.

void XYZLLATransNode::losts_callback(const expo_crowd_msgs::Losts &msg){
    expo_crowd_msgs::LostsFix ret_msg;
    ret_msg.header = msg.header;
    ret_msg.num = msg.num;
    ret_msg.header.frame_id = "";

    // フレーム抽出.
    std::string crowd_frame = "ylm";
    geometry_msgs::TransformStamped transformStamped;
    try{
        transformStamped = tfBuffer.lookupTransform(map_frame, crowd_frame, ros::Time(0));
    }
    catch (tf2::TransformException& ex){
        ROS_WARN("%s", ex.what());
        return;
    }

    // 各要素を変換する.
    for (const auto& lostitem : msg.lostitem) {

        // フレーム変換.
        geometry_msgs::Point pose_on_map;
        geometry_msgs::Point pose_on_lostsframe = lostitem.position;
        tf2::doTransform(pose_on_lostsframe, pose_on_map, transformStamped);

        // 緯度経度変換.
        Eigen::Vector3d pose;
        fix_xyz_trans::LatLonAlt lla;
        pose(0) = pose_on_map.x;
        pose(1) = pose_on_map.y;
        pose(2) = pose_on_map.z;
        lla = l_u_transformer.get_latlonalt_from_xyz(pose);

        expo_crowd_msgs::LostItemFix lostitem_fix;
        lostitem_fix.id = lostitem.id;
        lostitem_fix.latitude = lla.latitude;
        lostitem_fix.longitude = lla.longitude;
        lostitem_fix.altitude = lla.altitude;

        lostitem_fix.size = lostitem.size;
        lostitem_fix.type = lostitem.type;
        ret_msg.lostitem.push_back(lostitem_fix);
    }

    losts_fix_pub.publish(ret_msg);

    debug_msg = "published losts_fix";
    DEBUG_PRINT(debug_msg);
}

// 混雑度のコールバック.
void XYZLLATransNode::crowd_callback(const expo::CrowdEX &msg){
    expo_crowd_msgs::CrowdFixEX ret_msg;
    ret_msg.header = msg.header;
    ret_msg.header.frame_id = "";

    // フレーム抽出.
    std::string crowd_frame = msg.header.frame_id;
    geometry_msgs::TransformStamped transformStamped;
    try{
        transformStamped = tfBuffer.lookupTransform(map_frame, crowd_frame, ros::Time(0));
    }
    catch (tf2::TransformException& ex){
        ROS_WARN("%s", ex.what());
        return;
    }

    // 各要素を変換する.
    for (const auto& person : msg.persons) {

        // フレーム変換.
        geometry_msgs::Pose pose_on_map;
        geometry_msgs::Pose pose_on_crowdframe;
        pose_on_crowdframe.position = person.position;
        pose_on_crowdframe.orientation = person.orientation;
        tf2::doTransform(pose_on_crowdframe, pose_on_map, transformStamped);

        // 緯度経度変換.
        fix_xyz_trans::Pose pose;
        fix_xyz_trans::LLAWithOrientation lla_with_ori;
        pose.position(0) = pose_on_map.position.x;
        pose.position(1) = pose_on_map.position.y;
        pose.position(2) = pose_on_map.position.z;
        pose.orientation(0) = pose_on_map.orientation.x;
        pose.orientation(1) = pose_on_map.orientation.y;
        pose.orientation(2) = pose_on_map.orientation.z;
        pose.orientation(3) = pose_on_map.orientation.w;
        lla_with_ori = l_u_transformer.get_latlonalt_from_xyz(pose);

        expo_crowd_msgs::PersonArrowFixEX person_fix;
        person_fix.id = person.id;
        person_fix.latitude = lla_with_ori.lla.latitude;
        person_fix.longitude = lla_with_ori.lla.longitude;
        person_fix.altitude = lla_with_ori.lla.altitude;
        //person_fix.orientation = pose_on_map.orientation;

        person_fix.orientation.x = lla_with_ori.orientation(0);
        person_fix.orientation.y = lla_with_ori.orientation(1);
        person_fix.orientation.z = lla_with_ori.orientation(2);
        person_fix.orientation.w = lla_with_ori.orientation(3);

        person_fix.velocity = person.velocity;
        person_fix.state = person.state;
        ret_msg.persons.push_back(person_fix);
    }

    crowd_fix_pub.publish(ret_msg);

    debug_msg = "published crowd_fix";
    DEBUG_PRINT(debug_msg);
}

// odomコールバック関数.
void XYZLLATransNode::odom_callback1(const nav_msgs::Odometry &msg){
    debug_msg = "get odom_msg";
    DEBUG_PRINT(debug_msg);

    expo_fix_msgs::FixWithOrientation ret_msg;
    sensor_msgs::NavSatFix fix_msg;
    fix_xyz_trans::Pose pose;
    fix_xyz_trans::LLAWithOrientation lla_with_ori;
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

    pose.position(0) = pose_on_map.position.x ;
    pose.position(1) = pose_on_map.position.y ;
    pose.position(2) = pose_on_map.position.z ;
    pose.orientation(0) = pose_on_map.orientation.x;
    pose.orientation(1) = pose_on_map.orientation.y;
    pose.orientation(2) = pose_on_map.orientation.z;
    pose.orientation(3) = pose_on_map.orientation.w;

    lla_with_ori = l_u_transformer.get_latlonalt_from_xyz(pose);

    fix_msg.header = msg.header ;
    fix_msg.header.frame_id = "" ;
    fix_msg.latitude = lla_with_ori.lla.latitude;
    fix_msg.longitude = lla_with_ori.lla.longitude;
    fix_msg.altitude = lla_with_ori.lla.altitude;
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
    ret_msg.orientation.x = lla_with_ori.orientation(0);
    ret_msg.orientation.y = lla_with_ori.orientation(1);
    ret_msg.orientation.z = lla_with_ori.orientation(2);
    ret_msg.orientation.w = lla_with_ori.orientation(3);
    //ret_msg.orientation = pose_on_map.orientation;

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
    fix_xyz_trans::Pose pose;
    fix_xyz_trans::LLAWithOrientation lla_with_ori;

    pose.position(0) = msg.position.x ;
    pose.position(1) = msg.position.y ;
    pose.position(2) = msg.position.z ;
    pose.orientation(0) = msg.pose.x;
    pose.orientation(1) = msg.pose.y;
    pose.orientation(2) = msg.pose.z;
    pose.orientation(3) = msg.pose.w;

    lla_with_ori = l_u_transformer.get_latlonalt_from_xyz(pose);

    ret_msg.id = msg.id;
    ret_msg.latitude = lla_with_ori.lla.latitude;
    ret_msg.longitude = lla_with_ori.lla.longitude;
    ret_msg.altitude = lla_with_ori.lla.altitude;
    ret_msg.size = msg.size;
    ret_msg.velocity = msg.velocity;
    ret_msg.density = msg.density;
    //ret_msg.pose = msg.pose;
    ret_msg.pose.x = lla_with_ori.orientation(0);
    ret_msg.pose.y = lla_with_ori.orientation(1);
    ret_msg.pose.z = lla_with_ori.orientation(2);
    ret_msg.pose.w = lla_with_ori.orientation(3);
    
    
    area_fix_pub.publish(ret_msg);

    debug_msg = "published area_fix_msg from area_msg";
    DEBUG_PRINT(debug_msg);
}

void XYZLLATransNode::person_callback(const expo_msgs::Person &msg){
    debug_msg = "get person_msg";
    DEBUG_PRINT(debug_msg);

    expo_fix_msgs::PersonFix ret_msg;
    fix_xyz_trans::Pose pose;
    fix_xyz_trans::LLAWithOrientation lla_with_ori;

    pose.position(0) = msg.position.x ;
    pose.position(1) = msg.position.y ;
    pose.position(2) = msg.position.z ;
    pose.orientation(0) = msg.orientation.x;
    pose.orientation(1) = msg.orientation.y;
    pose.orientation(2) = msg.orientation.z;
    pose.orientation(3) = msg.orientation.w;

    lla_with_ori = l_u_transformer.get_latlonalt_from_xyz(pose);

    ret_msg.latitude = lla_with_ori.lla.latitude;
    ret_msg.longitude = lla_with_ori.lla.longitude;
    ret_msg.altitude = lla_with_ori.lla.altitude;
    //ret_msg.orientation = msg.orientation;
    ret_msg.orientation.x = lla_with_ori.orientation(0);
    ret_msg.orientation.y = lla_with_ori.orientation(1);
    ret_msg.orientation.z = lla_with_ori.orientation(2);
    ret_msg.orientation.w = lla_with_ori.orientation(3);
    
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
    sub_losts_topic = "losts";
    sub_crowd_topic = "crowd";
    sub_odom_topic1 = "odom1";
    sub_pose_topic2 = "pose2";
    sub_posecov_topic3 = "posecov3";
    sub_area_topic = "area";
    sub_person_topic = "person";
    sub_maigo_topic = "maigo";
    sub_otosimono_topic = "otosimono";

    pub_losts_fix_topic = "lost_fix";
    pub_crowd_fix_topic = "crowd_fix";
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
    double qx,qy,qz,qw;
    
    
    // rosparamの取得.
    pnh.getParam("sub_losts_topic", sub_losts_topic);
    pnh.getParam("sub_crowd_topic", sub_crowd_topic);
    pnh.getParam("sub_odom_topic1", sub_odom_topic1);
    pnh.getParam("sub_pose_topic2", sub_pose_topic2);
    pnh.getParam("sub_posecov_topic3", sub_posecov_topic3);
    pnh.getParam("sub_area_topic", sub_area_topic);
    pnh.getParam("sub_person_topic", sub_person_topic);
    pnh.getParam("sub_maigo_topic", sub_maigo_topic);
    pnh.getParam("sub_otosimono_topic", sub_otosimono_topic);
    
    pnh.getParam("pub_losts_fix_topic", pub_losts_fix_topic);
    pnh.getParam("pub_crowd_fix_topic", pub_crowd_fix_topic);
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

    if (pnh.getParam("origin_pose", origin_pose_str)){
        std::replace(origin_pose_str.begin(), origin_pose_str.end(), ',', ' ');
        std::istringstream iss_pose(origin_pose_str);
        iss_pose >> origin_latlonalt_.latitude >> origin_latlonalt_.longitude >> origin_latlonalt_.altitude;
    }
    if (pnh.getParam("origin_quat", origin_quat_str)){
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
    }
    else if(pnh.getParam("origin_quat_inv", origin_quat_str)){
        
        std::replace(origin_quat_str.begin(), origin_quat_str.end(), ',', ' ');
        std::istringstream iss_quat(origin_quat_str);
        iss_quat >> qx >> qy >> qz >> qw;
        
        origin_quat_ << -qx, -qy, -qz, qw;
        if (origin_quat_.norm() == 0){
            std::cerr << "invalid quaternion" << std::endl;
            origin_quat_ << 0.0 , 0.0 , 0.0 , 1.0;
        }
        else{
            origin_quat_.normalize();
        }
    }
    l_u_transformer.set_origin(origin_latlonalt_, origin_quat_);

    // publisher,subscriberの設定.
    losts_sub = nh.subscribe(sub_losts_topic, 10, &XYZLLATransNode::losts_callback, this);
    crowd_sub = nh.subscribe(sub_crowd_topic, 10, &XYZLLATransNode::crowd_callback, this);
    odom_sub1 = nh.subscribe(sub_odom_topic1, 10, &XYZLLATransNode::odom_callback1, this);
    pose_sub2 = nh.subscribe(sub_pose_topic2, 10, &XYZLLATransNode::pose_callback2, this);
    posecov_sub3 = nh.subscribe(sub_posecov_topic3, 10, &XYZLLATransNode::posecov_callback3, this);
    area_sub = nh.subscribe(sub_area_topic, 10, &XYZLLATransNode::area_callback, this);
    person_sub = nh.subscribe(sub_person_topic, 10, &XYZLLATransNode::person_callback, this);
    maigo_sub = nh.subscribe(sub_maigo_topic, 10, &XYZLLATransNode::maigo_callback, this);
    otosimono_sub = nh.subscribe(sub_otosimono_topic, 10, &XYZLLATransNode::otosimono_callback, this);

    losts_fix_pub = nh.advertise<expo_crowd_msgs::LostsFix>(pub_losts_fix_topic, 10);
    crowd_fix_pub = nh.advertise<expo_crowd_msgs::CrowdFixEX>(pub_crowd_fix_topic, 10);
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
