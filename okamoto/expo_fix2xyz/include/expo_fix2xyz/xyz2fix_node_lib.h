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
#include <expo_fix_msgs/FixWithOrientation.h>
#include <expo_crowd_msgs/CrowdEX.h>
#include <expo_crowd_msgs/CrowdFixEX.h>
#include <expo_crowd_msgs/Losts.h>
#include <expo_crowd_msgs/LostsFix.h>


#include <expo/Person.h>
#include <expo/CrowdEX.h>

#include <proj.h>
#include "expo_fix2xyz/fix_xyz_transform.h"

using namespace fix_xyz_trans;

class XYZLLATransNode {
  private:
    
    LLAXYZTrans l_u_transformer;
    tf2_ros::Buffer tfBuffer;
    tf2_ros::TransformListener tfListener;

    std::string debug_msg;

    ros::NodeHandle nh;
    ros::NodeHandle pnh;
    
    ros::Subscriber losts_sub;
    ros::Subscriber crowd_sub;
    ros::Subscriber maigo_sub;
    ros::Subscriber otosimono_sub;
    ros::Subscriber area_sub;
    ros::Subscriber person_sub;
    ros::Subscriber odom_sub1;   // -> publisher fix_pub1
    ros::Subscriber pose_sub2;   // -> publisher fix_pub2
    ros::Subscriber posecov_sub3;// -> publisher fix_pub3

    ros::Publisher losts_fix_pub;
    ros::Publisher crowd_fix_pub;
    ros::Publisher maigo_fix_pub;
    ros::Publisher otosimono_fix_pub;
    ros::Publisher area_fix_pub;
    ros::Publisher person_fix_pub;
    ros::Publisher fix_pub1;
    ros::Publisher fix_pub2;
    ros::Publisher fix_pub3;

    std::string sub_losts_topic;
    std::string sub_crowd_topic;
    std::string sub_odom_topic1;
    std::string sub_pose_topic2;
    std::string sub_posecov_topic3;
    std::string sub_area_topic;
    std::string sub_person_topic;
    std::string sub_maigo_topic;
    std::string sub_otosimono_topic;
    
    std::string pub_losts_fix_topic;
    std::string pub_crowd_fix_topic;
    std::string pub_fix_topic1;
    std::string pub_fix_topic2;
    std::string pub_fix_topic3;
    std::string pub_area_fix_topic;
    std::string pub_person_fix_topic;
    std::string pub_maigo_fix_topic;
    std::string pub_otosimono_fix_topic;


    std::string origin_pose_str;
    std::string origin_quat_str;
    std::string map_frame;

    int epsg_code_num;

  public:
    // コールバック関数.
    void losts_callback(const expo_crowd_msgs::Losts &msg);
    void crowd_callback(const expo::CrowdEX &msg);
    void odom_callback1(const nav_msgs::Odometry &msg);
    void pose_callback2(const geometry_msgs::PoseStamped &msg);
    void posecov_callback3(const geometry_msgs::PoseWithCovarianceStamped &msg);
    void area_callback(const expo_msgs::Area &msg);
    void person_callback(const expo_msgs::Person &msg);
    void maigo_callback(const geometry_msgs::Point &msg);
    void otosimono_callback(const geometry_msgs::Point &msg);

    XYZLLATransNode();
    ~XYZLLATransNode();


};

#endif
