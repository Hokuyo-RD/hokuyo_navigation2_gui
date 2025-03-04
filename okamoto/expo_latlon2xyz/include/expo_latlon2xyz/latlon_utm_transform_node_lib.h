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

#include <proj.h>
#include "expo_latlon2xyz/latlon_utm_transform.h"

using namespace latlon_utm_trans;

class LatlonUtmTransNode {
  private:
    
    LatlonUtmTrans l_u_transformer;

    std::string debug_msg;

    ros::NodeHandle nh;
    ros::NodeHandle pnh;
    
    ros::Subscriber latlon_sub1;
    ros::Subscriber latlon_sub2;
    ros::Subscriber latlon_sub3;
    ros::Subscriber utm_sub1;
    ros::Subscriber utm_sub2;
    ros::Subscriber utm_sub3;

    ros::Publisher latlon_pub1;
    ros::Publisher latlon_pub2;
    ros::Publisher latlon_pub3;
    ros::Publisher utm_pub1;
    ros::Publisher utm_pub2;
    ros::Publisher utm_pub3;

    std::string sub_latlon_topic1;
    std::string sub_latlon_topic2;
    std::string sub_latlon_topic3;
    std::string sub_utm_topic1;
    std::string sub_utm_topic2;
    std::string sub_utm_topic3;
    
    std::string pub_latlon_topic1;
    std::string pub_latlon_topic2;
    std::string pub_latlon_topic3;
    std::string pub_utm_topic1;
    std::string pub_utm_topic2;
    std::string pub_utm_topic3;

    std::string origin_pose_str;
    std::string origin_quat_str;

    int epsg_code_num;
    double rot_cov;
    bool make_angle_from_movement;
    
    bool latlon_isfirst;
    geometry_msgs::Pose last_latlon_pose;

    nav_msgs::Odometry sub_utm_msg;
    sensor_msgs::NavSatFix sub_latlon_msg;


  public:
    // utmコールバック関数.
    void utm_callback1(const nav_msgs::Odometry &msg);
    void utm_callback2(const nav_msgs::Odometry &msg);
    void utm_callback3(const nav_msgs::Odometry &msg);

    // latlonコールバック関数.
    void latlon_callback1(const sensor_msgs::NavSatFix &msg);
    void latlon_callback2(const sensor_msgs::NavSatFix &msg);
    void latlon_callback3(const sensor_msgs::NavSatFix &msg);

    LatlonUtmTransNode();
    ~LatlonUtmTransNode();


};

#endif
