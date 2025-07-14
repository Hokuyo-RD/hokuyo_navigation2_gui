# include <ros/ros.h>
# include <geometry_msgs/Twist.h>
# include <std_msgs/Int32.h>
# include <std_msgs/Empty.h>
#include <sensor_msgs/LaserScan.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/NavSatFix.h>
#include <safety_data_message/SafetyData.h>

# include <string>

class CmdVelStopper{
    private:
        ros::NodeHandle _nh;
        ros::NodeHandle _nhPrivate;
        ros::Subscriber _sub_cmd;
        ros::Subscriber _sub_stop;
        ros::Subscriber _sub_start;
        ros::Publisher _pub_cmd;
        bool _stop_cmdvel;
        geometry_msgs::Twist twist_zero;

    public:
        CmdVelStopper();
        void callbackCMD(const geometry_msgs::Twist& msgs);
        void callbackStart(const std_msgs::Empty& msgs);
        void callbackStop(const std_msgs::Empty& msgs);
};

CmdVelStopper::CmdVelStopper()
    : _nhPrivate("~")
{
    twist_zero.linear.x = 0;
    twist_zero.linear.y = 0;
    twist_zero.linear.z = 0;
    twist_zero.angular.x = 0;
    twist_zero.angular.y = 0;
    twist_zero.angular.z = 0;

    std::string cmd_in_topic = "/wizurg_tmp/cmd_vel";
    std::string cmd_out_topic = "/icart_mini/cmd_vel";
    std::string cmd_stop_topic = "/wizurg/stop_cmd_vel";
    std::string cmd_start_topic = "/wizurg/start_cmd_vel";
    
    _stop_cmdvel = false;
    
    _nhPrivate.getParam("cmd_in_topic",cmd_in_topic);
    _nhPrivate.getParam("cmd_out_topic", cmd_out_topic);
    _nhPrivate.getParam("top_topic",cmd_stop_topic);
    _nhPrivate.getParam("start_topic", cmd_start_topic);

    _sub_cmd = _nh.subscribe(cmd_in_topic, 10, &CmdVelStopper::callbackCMD, this);
    _sub_stop = _nh.subscribe(cmd_stop_topic, 10, &CmdVelStopper::callbackStop, this);
    _sub_start = _nh.subscribe(cmd_start_topic, 10, &CmdVelStopper::callbackStart, this);
    
    _pub_cmd = _nh.advertise<geometry_msgs::Twist>(cmd_out_topic, 10);

}

void CmdVelStopper::callbackStop(const std_msgs::Empty& msgs){
    _stop_cmdvel = true;
}

void CmdVelStopper::callbackStart(const std_msgs::Empty& msgs){
    _stop_cmdvel = false;
}

void CmdVelStopper::callbackCMD(const geometry_msgs::Twist& msgs)
{
    if(_stop_cmdvel){
        _pub_cmd.publish(twist_zero);
    }
    else{
        _pub_cmd.publish(msgs);
    }
}

int main(int argc, char **argv){
    ros::init(argc, argv, "expo_cmdvel_stopper");
    CmdVelStopper cmd_vel_stopper;
    ros::spin();
    return 0;
}

