#!/usr/bin/env python3
import rospy
import subprocess
from std_msgs.msg import String

trigger_topic = String()
bash_fullpath = String()

def callback(data):
    print(trigger_topic)
    subprocess.run([bash_fullpath,"arg1","arg2","..."])

def listener():
    global trigger_topic
    global bash_fullpath
    rospy.init_node('bash_trigger', anonymous=False)
    trigger_topic = rospy.get_param("~trigger_topic","/trigger")
    bash_fullpath = rospy.get_param("~bash_fullpath","/home/ubuntu/catkin_ws/src/expo_software/okamoto/expo_bash_trigger/scripts/test.sh")
    rospy.Subscriber(trigger_topic, String, callback)
    rospy.spin()

if __name__ == '__main__':
    listener()