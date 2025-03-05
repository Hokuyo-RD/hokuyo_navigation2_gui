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

#include <proj.h>
#include "expo_latlon2xyz/latlon_utm_transform.h"

using namespace latlon_utm_trans;

class LatlonUtmTransNode {
  private:
    
    LatlonUtmTrans l_u_transformer;
    tf2_ros::Buffer tfBuffer;

    std::string debug_msg;

    ros::NodeHandle nh;
    ros::NodeHandle pnh;
    
    ros::Subscriber latlon_sub1; // -> publisher odom_pub1 
    ros::Subscriber latlon_sub2; // -> publisher pose_pub2
    ros::Subscriber latlon_sub3; // -> publisher posecov_pub3
    ros::Subscriber odom_sub1;   // -> publisher latlon_pub1
    ros::Subscriber pose_sub2;   // -> publisher latlon_pub2
    ros::Subscriber posecov_sub3;// -> publisher latlon_pub3

    ros::Publisher latlon_pub1;
    ros::Publisher latlon_pub2;
    ros::Publisher latlon_pub3;
    ros::Publisher odom_pub1;
    ros::Publisher pose_pub2;
    ros::Publisher posecov_pub3;

    std::string sub_latlon_topic1;
    std::string sub_latlon_topic2;
    std::string sub_latlon_topic3;
    std::string sub_odom_topic1;
    std::string sub_pose_topic2;
    std::string sub_posecov_topic3;
    
    std::string pub_latlon_topic1;
    std::string pub_latlon_topic2;
    std::string pub_latlon_topic3;
    std::string pub_odom_topic1;
    std::string pub_pose_topic2;
    std::string pub_posecov_topic3;

    std::string origin_pose_str;
    std::string origin_quat_str;
    std::string map_frame;

    int epsg_code_num;
    double rot_cov;
    bool make_angle_from_movement;
    
    bool latlon_isfirst;
    geometry_msgs::Pose last_latlon_pose;

  public:
    // utmコールバック関数.
    void odom_callback1(const nav_msgs::Odometry &msg);
    void pose_callback2(const geometry_msgs::PoseStamped &msg);
    void posecov_callback3(const geometry_msgs::PoseWithCovarianceStamped &msg);

    // latlonコールバック関数.
    void latlon_callback1(const sensor_msgs::NavSatFix &msg);
    void latlon_callback2(const sensor_msgs::NavSatFix &msg);
    void latlon_callback3(const sensor_msgs::NavSatFix &msg);

    LatlonUtmTransNode();
    ~LatlonUtmTransNode();


};

#endif
