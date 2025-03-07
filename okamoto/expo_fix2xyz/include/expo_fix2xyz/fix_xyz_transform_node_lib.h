#ifndef LATLON_UTM_TRANS_NODE
#define LATLON_UTM_TRANS_NODE


#ifdef DEBUG_MODE
#define DEBUG_PRINT(A) std::cout << (#A) << " : " << (A) << std::endl;
#else
#define DEBUG_PRINT(A) {;}
#endif

#include <cmath>
#include <ros/ros.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/String.h>
#include <sensor_msgs/NavSatFix.h>
#include <geometry_msgs/Pose.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <expo_msgs/Area.h>
#include <expo_msgs/Person.h>
#include <expo_fix_msgs/AreaFix.h>
#include <expo_fix_msgs/PersonFix.h>

#include <proj.h>
#include "expo_fix2xyz/fix_xyz_transform.h"

using namespace fix_xyz_trans;

class LatlonUtmTransNode {
  private:
    
    LatlonUtmTrans l_u_transformer;
    tf2_ros::Buffer tfBuffer;

    std::string debug_msg;

    ros::NodeHandle nh;
    ros::NodeHandle pnh;
    
    ros::Subscriber fix_sub1; // -> publisher odom_pub1 
    ros::Subscriber fix_sub2; // -> publisher pose_pub2
    ros::Subscriber fix_sub3; // -> publisher posecov_pub3
    ros::Subscriber odom_sub1;   // -> publisher fix_pub1
    ros::Subscriber pose_sub2;   // -> publisher fix_pub2
    ros::Subscriber posecov_sub3;// -> publisher fix_pub3
    ros::Subscriber area_sub;
    ros::Subscriber person_sub;
    ros::Subscriber area_fix_sub;
    ros::Subscriber person_fix_sub;

    ros::Publisher fix_pub1;
    ros::Publisher fix_pub2;
    ros::Publisher fix_pub3;
    ros::Publisher odom_pub1;
    ros::Publisher pose_pub2;
    ros::Publisher posecov_pub3;
    ros::Publisher area_pub;
    ros::Publisher person_pub;
    ros::Publisher area_fix_pub;
    ros::Publisher person_fix_pub;

    std::string sub_fix_topic1;
    std::string sub_fix_topic2;
    std::string sub_fix_topic3;
    std::string sub_odom_topic1;
    std::string sub_pose_topic2;
    std::string sub_posecov_topic3;
    std::string sub_area_topic;
    std::string sub_person_topic;
    std::string sub_area_fix_topic;
    std::string sub_person_fix_topic;
    
    std::string pub_fix_topic1;
    std::string pub_fix_topic2;
    std::string pub_fix_topic3;
    std::string pub_odom_topic1;
    std::string pub_pose_topic2;
    std::string pub_posecov_topic3;
    std::string pub_area_topic;
    std::string pub_person_topic;
    std::string pub_area_fix_topic;
    std::string pub_person_fix_topic;

    std::string origin_pose_str;
    std::string origin_quat_str;
    std::string map_frame;

    int epsg_code_num;
    double rot_cov;
    bool make_angle_from_movement;
    
    bool fix1_isfirst;
    bool fix2_isfirst;
    bool fix3_isfirst;
    geometry_msgs::Pose last_fix1_pose;
    geometry_msgs::Pose last_fix2_pose;
    geometry_msgs::Pose last_fix3_pose;

  public:
    // コールバック関数.
    void odom_callback1(const nav_msgs::Odometry &msg);
    void pose_callback2(const geometry_msgs::PoseStamped &msg);
    void posecov_callback3(const geometry_msgs::PoseWithCovarianceStamped &msg);
    void fix_callback1(const sensor_msgs::NavSatFix &msg);
    void fix_callback2(const sensor_msgs::NavSatFix &msg);
    void fix_callback3(const sensor_msgs::NavSatFix &msg);
    void area_callback(const expo_msgs::Area &msg);
    void person_callback(const expo_msgs::Person &msg);
    void area_fix_callback(const expo_fix_msgs::AreaFix &msg);
    void person_fix_callback(const expo_fix_msgs::PersonFix &msg);

    LatlonUtmTransNode();
    ~LatlonUtmTransNode();


};

#endif
