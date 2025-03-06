#!/usr/bin/env python3
import rospy
from geometry_msgs.msg import Point
from expo_msgs.msg import Area
from message_filters import Subscriber, ApproximateTimeSynchronizer

def callback(data):
        id_value = data.id 
        position_x = data.position.x 
        position_y = data.position.y
        position_z = data.position.z
        size = data.size
        velocity = data.velocity
        density = data.density
        pose_x = data.pose.x
        pose_y = data.pose.y
        pose_z = data.pose.z
        pose_w = data.pose.w

        
        rospy.loginfo(f"Subscribing: ID={id_value},Position_x = {position_x},Position_y = {position_y},Position_z = {position_z},Size = {size}, Velocity={velocity}, Density={density},Pose_x = {pose_x},Pose_y = {pose_y},Pose_z = {pose_z},Pose_w = {pose_w}")
        if density > 5:
             print("混雑")

def subscriber():
    rospy.init_node('geometry_subscriber', anonymous=True)
    rospy.Subscriber('data_topic', Area, callback)
    rospy.spin()
if __name__ == '__main__':
    try:
        subscriber()
    except rospy.ROSInterruptException:
        pass