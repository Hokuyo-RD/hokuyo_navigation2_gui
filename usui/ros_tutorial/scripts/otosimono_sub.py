#!/usr/bin/env python3
import rospy
from geometry_msgs.msg import Point
from expo_msgs.msg import Area
from message_filters import Subscriber, ApproximateTimeSynchronizer

def callback(data):
        position_x = data.x 
        position_y = data.y
        position_z = data.z
        print("callback")
        rospy.loginfo(f"[otosimono] position=({position_x}, {position_y}, {position_z})")


def subscriber():
    rospy.init_node('otosimono_subscriber', anonymous=True)
    rospy.Subscriber('otosimono', Point, callback)
    rospy.spin()  # コールバックを維持

if __name__ == '__main__':
    try:
        subscriber()
    except rospy.ROSInterruptException:
        pass