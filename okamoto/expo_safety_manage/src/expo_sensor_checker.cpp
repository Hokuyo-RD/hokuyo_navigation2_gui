# include <ros/ros.h>
# include <geometry_msgs/Twist.h>

# include <std_msgs/Int32.h>
#include <sensor_msgs/LaserScan.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/NavSatFix.h>
#include <safety_data_message/SafetyData.h>

# include <expo_safety_manage/SensorsState.h>
# include <string>

class SensorChecker{
    private:
        ros::NodeHandle _nh;
        ros::NodeHandle _nhPrivate;
        ros::Subscriber _sub_uam1;
        ros::Subscriber _sub_uam2;
        ros::Subscriber _sub_ust1;
        ros::Subscriber _sub_ust2;
        ros::Subscriber _sub_yvt;
        ros::Subscriber _sub_ylm;
        ros::Subscriber _sub_gnss;

        ros::Publisher _pub_sensor_alarm;
        ros::Publisher _pub_sensors_state;
        ros::Timer interval_pub;
        expo_safety_manage::SensorsState _sensors_state;
        double interval_time;

        void reset_states();

    public:
        SensorChecker();
        void callbackUAM1(const safety_data_message::SafetyData& msgs);
        void callbackUAM2(const safety_data_message::SafetyData& msgs);
        void callbackUST1(const sensor_msgs::LaserScan& msgs);
        void callbackUST2(const sensor_msgs::LaserScan& msgs);
        void callbackYVT(const sensor_msgs::PointCloud2& msgs);
        void callbackYLM(const sensor_msgs::PointCloud2& msgs);
        void callbackGNSS(const sensor_msgs::NavSatFix& msgs);
        void interval_callback(const ros::TimerEvent &event);
};

SensorChecker::SensorChecker()
    : _nhPrivate("~")
{
    std::string uam1_topic = "/uam1";
    std::string uam2_topic = "/uam2";
    std::string ust1_topic = "/urg_node1/scan";
    std::string ust2_topic = "/urg_node2/scan";
    std::string yvt_topic = "/hokuyo3d3/hokuyo_cloud2";
    std::string ylm_topic = "/lumotive_ros/pointcloud";
    std::string gnss_topic = "/fix";
    std::string alarm_topic = "/sensor_lost_alarm";
    std::string state_topic = "/sensors_state";
    interval_time = 2;

    _nhPrivate.getParam("uam1_topic", uam1_topic);
    _nhPrivate.getParam("uam2_topic", uam2_topic);
    _nhPrivate.getParam("ust1_topic", ust1_topic);
    _nhPrivate.getParam("ust2_topic", ust2_topic);
    _nhPrivate.getParam("yvt_topic", yvt_topic);
    _nhPrivate.getParam("ylm_topic", ylm_topic);
    _nhPrivate.getParam("gnss_topic", gnss_topic);
    _nhPrivate.getParam("alarm_topic", alarm_topic);
    _nhPrivate.getParam("state_topic", state_topic);
    _nhPrivate.getParam("interval_time", interval_time);

    _sub_uam1 = _nh.subscribe(uam1_topic, 10, &SensorChecker::callbackUAM1, this);
    _sub_uam2 = _nh.subscribe(uam2_topic, 10, &SensorChecker::callbackUAM2, this);
    _sub_ust1 = _nh.subscribe(ust1_topic, 10, &SensorChecker::callbackUST1, this);
    _sub_ust2 = _nh.subscribe(ust2_topic, 10, &SensorChecker::callbackUST2, this);
    _sub_yvt = _nh.subscribe(yvt_topic, 10, &SensorChecker::callbackYVT, this);
    _sub_ylm = _nh.subscribe(ylm_topic, 10, &SensorChecker::callbackYLM, this);
    _sub_gnss = _nh.subscribe(gnss_topic, 10, &SensorChecker::callbackGNSS, this);

    _pub_sensor_alarm = _nh.advertise<std_msgs::Int32>(alarm_topic, 10);
    _pub_sensors_state = _nh.advertise<expo_safety_manage::SensorsState>(state_topic, 10);

    interval_pub = _nh.createTimer(ros::Duration(interval_time), &SensorChecker::interval_callback, this);

    reset_states();
}

void SensorChecker::reset_states(){
    _sensors_state.uam1 = false;
    _sensors_state.uam2 = false;
    _sensors_state.ust1 = false;
    _sensors_state.ust2 = false;
    _sensors_state.yvt = false;
    _sensors_state.ylm = false;
    _sensors_state.gnss = false;
}

void SensorChecker::callbackUAM1(const safety_data_message::SafetyData& msgs){
    _sensors_state.uam1 = true;
}
void SensorChecker::callbackUAM2(const safety_data_message::SafetyData& msgs){
    _sensors_state.uam2 = true;
}
void SensorChecker::callbackUST1(const sensor_msgs::LaserScan& msgs){
    _sensors_state.ust1 = true;
}
void SensorChecker::callbackUST2(const sensor_msgs::LaserScan& msgs){
    _sensors_state.ust2 = true;
}
void SensorChecker::callbackYVT(const sensor_msgs::PointCloud2& msg){
    _sensors_state.yvt = true;
}
void SensorChecker::callbackYLM(const sensor_msgs::PointCloud2& msg){
    _sensors_state.ylm = true;
}
void SensorChecker::callbackGNSS(const sensor_msgs::NavSatFix& msgs){
    _sensors_state.gnss = true;
}
void SensorChecker::interval_callback(const ros::TimerEvent &event){
    std_msgs::Int32 alarm_msg;
    
    // トピック有無の確認..
    if(!_sensors_state.yvt || !_sensors_state.uam1 || !_sensors_state.uam2 ){
        alarm_msg.data = 2; //異常あり.
    }
    else if(!_sensors_state.ust1 || !_sensors_state.ust2 || !_sensors_state.gnss ){
        alarm_msg.data = 1; //注意.
    }
    else{
        alarm_msg.data = 0; //正常.
    }

    _sensors_state.header.stamp = ros::Time::now();
    _pub_sensors_state.publish(_sensors_state);
    _pub_sensor_alarm.publish(alarm_msg);
    reset_states();
}


int main(int argc, char **argv){
    ros::init(argc, argv, "expo_sensor_checker");
    SensorChecker SensorChecker;
    ros::spin();
    return 0;
}

