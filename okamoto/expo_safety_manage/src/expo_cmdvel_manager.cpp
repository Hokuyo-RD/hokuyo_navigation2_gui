# include <ros/ros.h>
# include <geometry_msgs/Twist.h>
# include <std_msgs/Int16.h>
#include <sensor_msgs/LaserScan.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/NavSatFix.h>
#include <safety_data_message/SafetyData.h>

# include <string>

class CmdVelManager{
    private:
        ros::NodeHandle _nh;
        ros::NodeHandle _nhPrivate;
        ros::Subscriber _sub_cmd;
        ros::Subscriber _sub_uam1;
        ros::Subscriber _sub_uam2;
        ros::Publisher _pub_cmd;
        ros::Publisher _pub_approach_alarm;
        ros::Timer ros_interval;
        bool _uam1_is_safe;
        bool _uam2_is_safe;
        bool _uam1_msg_received;
        bool _uam2_msg_received;
        bool _stop_manage;
        std_msgs::Int16 alarm;
        int uam1_alarm;
        int uam2_alarm;
        double interval_time;
        geometry_msgs::Twist twist_zero;

    public:
        CmdVelManager();
        void callbackCMD(const geometry_msgs::Twist msgs);
        void callbackUAM1(const safety_data_message::SafetyData& msgs);
        void callbackUAM2(const safety_data_message::SafetyData& msgs);
        void publish_alarm();
        void interval_callback(const ros::TimerEvent &event);
};

CmdVelManager::CmdVelManager()
    : _nhPrivate("~")
{
    alarm.data = 0;
    uam1_alarm = 0;
    uam2_alarm = 0;
    twist_zero.linear.x = 0;
    twist_zero.linear.y = 0;
    twist_zero.linear.z = 0;
    twist_zero.angular.x = 0;
    twist_zero.angular.y = 0;
    twist_zero.angular.z = 0;

    std::string cmd_in_topic = "/icart_mini/cmd_vel";
    std::string uam1_topic = "/uam1";
    std::string uam2_topic = "/uam2";
    std::string apploach_alarm_topic = "/apploach_alarm";
    std::string cmd_out_topic = "/wizurg/cmd_vel";
    interval_time = 0.5;
    
    _uam1_is_safe = false;
    _uam2_is_safe = false;
    _uam1_msg_received = false;
    _uam2_msg_received = false;
    _stop_manage = false;
    
    _nhPrivate.getParam("cmd_in_topic",cmd_in_topic);
    _nhPrivate.getParam("cmd_out_topic", cmd_out_topic);
    _nhPrivate.getParam("uam1_topic", uam1_topic);
    _nhPrivate.getParam("uam2_topic", uam2_topic);
    _nhPrivate.getParam("apploach_alarm_topic", apploach_alarm_topic);
    _nhPrivate.getParam("stop_manage", _stop_manage);
    _nhPrivate.getParam("interval_time", interval_time);
    
    _sub_cmd = _nh.subscribe(cmd_in_topic, 10, &CmdVelManager::callbackCMD, this);
    _sub_uam1 = _nh.subscribe(uam1_topic, 10, &CmdVelManager::callbackUAM1, this);
    _sub_uam2 = _nh.subscribe(uam2_topic, 10, &CmdVelManager::callbackUAM2, this);
    
    _pub_cmd = _nh.advertise<geometry_msgs::Twist>(cmd_out_topic, 10);
    _pub_approach_alarm = _nh.advertise<std_msgs::Int16>(apploach_alarm_topic, 10);

    ros_interval = _nh.createTimer(ros::Duration(interval_time), &CmdVelManager::interval_callback, this);
}

void CmdVelManager::callbackCMD(const geometry_msgs::Twist msgs)
{
    geometry_msgs::Twist pub_msg;

    
    if(uam1_alarm == 2 && uam2_alarm == 2){
        pub_msg = twist_zero;
    }
    else{
        pub_msg = msgs;
    }

    if(_stop_manage){
        pub_msg = msgs;
    }

    _pub_cmd.publish(pub_msg);
}

void CmdVelManager::callbackUAM1(const safety_data_message::SafetyData& msgs)
{
    _uam1_msg_received = true;

    if(msgs.ossd_1_status == 1){ // 危険エリア内に物体を検知.
        uam1_alarm = 2;
    }
    else{
        if(msgs.warning_1_status == 1){ // 注意エリア内に物体を検知.
            uam1_alarm = 1;
        }
        else{
            uam1_alarm = 0;
        }
    }
    publish_alarm();
}

void CmdVelManager::callbackUAM2(const safety_data_message::SafetyData& msgs)
{
    _uam2_msg_received = true;

    if(msgs.ossd_1_status == 1){ // 危険エリア内に物体を検知.
        uam2_alarm = 2;
    }
    else{
        if(msgs.warning_1_status == 1){ // 注意エリア内に物体を検知.
            uam2_alarm = 1;
        }
        else{
            uam2_alarm = 0;
        }
    }
    publish_alarm();
}

void CmdVelManager::publish_alarm(){
    if(uam2_alarm > uam1_alarm){ // より危険な方のアラームを出力する.
        alarm.data = uam2_alarm;
    }
    else{
        alarm.data = uam1_alarm;
    }
    std::cout << "publish_alarm" << std::endl;

    _pub_approach_alarm.publish(alarm);
}

void CmdVelManager::interval_callback(const ros::TimerEvent &event){

    // トピックがない場合、危険エリア判定にする.
    if(!_uam1_msg_received){
        uam1_alarm = 2;
    }
    if(!_uam2_msg_received){
        uam2_alarm = 2;
    }
    // 危険エリアの場合、（cmd_velをsubscribeしてなくても）停止命令をpublishする.
    if(uam1_alarm == 2 || uam2_alarm == 2){
        _pub_cmd.publish(twist_zero);
    }
    _uam1_msg_received = false;
    _uam2_msg_received = false;
}

int main(int argc, char **argv){
    ros::init(argc, argv, "expo_cmdvel_manager");
    CmdVelManager CmdVelManager;
    ros::spin();
    return 0;
}

