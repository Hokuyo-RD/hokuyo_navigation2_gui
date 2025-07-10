# include <ros/ros.h>
# include <expo_fix_msgs/FixWithOrientation.h>
# include <nav_msgs/Odometry.h>
#include <sensor_msgs/NavSatFix.h>

# include <string>

class FixOrientPublisher{
    private:
        ros::NodeHandle _nh;
        ros::NodeHandle _nhPrivate;
        ros::Subscriber _odometry_sub;
        ros::Subscriber _fix_sub;
        ros::Publisher _fix_with_orient_pub;
        
        
        ros::Time last_alert_time;
        nav_msgs::Odometry utm_odom;
        bool utm_odom_is_initialized;

    public:
        FixOrientPublisher();
        void callbackOdometry(const nav_msgs::Odometry& msgs);
        void callbackFix(const sensor_msgs::NavSatFix& msgs);
};

FixOrientPublisher::FixOrientPublisher()
    : _nh() , _nhPrivate("~")
{
    utm_odom_is_initialized = false;

    std::string utm_odom_topic = "/odometry/utm";
    std::string fix_topic = "/fix/switch";
    std::string fix_with_orient_topic = "/fix_with_orient/switch";
    
    _nhPrivate.getParam("utm_odom_topic",utm_odom_topic);
    _nhPrivate.getParam("fix_topic", fix_topic);
    _nhPrivate.getParam("fix_with_orient_topic", fix_with_orient_topic);

    _odometry_sub = _nh.subscribe(utm_odom_topic, 10, &FixOrientPublisher::callbackOdometry, this);
    _fix_sub = _nh.subscribe(fix_topic, 10, &FixOrientPublisher::callbackFix, this);
    
    _fix_with_orient_pub = _nh.advertise<expo_fix_msgs::FixWithOrientation>(fix_with_orient_topic, 10);
}

void FixOrientPublisher::callbackOdometry(const nav_msgs::Odometry& msgs)
{
    utm_odom = msgs;
    utm_odom_is_initialized = true;
}

void FixOrientPublisher::callbackFix(const sensor_msgs::NavSatFix& msgs)
{
    if(utm_odom_is_initialized){
        expo_fix_msgs::FixWithOrientation pub_msg;
        pub_msg.fix = msgs;
        pub_msg.orientation = utm_odom.pose.pose.orientation;
        _fix_with_orient_pub.publish(pub_msg);
    }
}

int main(int argc, char **argv){
    ros::init(argc, argv, "fix_odom_to_fix_orient");
    FixOrientPublisher FixOrientPublisher;
    ros::spin();
    return 0;
}

