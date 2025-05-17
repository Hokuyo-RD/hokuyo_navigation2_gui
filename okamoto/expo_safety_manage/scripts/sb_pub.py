#!/usr/bin/env python3
import rospy
from sensor_msgs.msg import NavSatFix
from geometry_msgs.msg import Quaternion
from sensor_msgs.msg import BatteryState 
from std_msgs.msg import UInt16
from std_msgs.msg import String

from expo_fix_msgs.msg import FixWithOrientation

get_new_odom = False

position_msg = NavSatFix()
orientation_msg = Quaternion()

floor_name_msg = String()
floor_name_msg.data = "banpaku" #(仮)

battery_state_msg = BatteryState()
battery_state_msg.capacity  = 100 #(仮)

robot_status_msg = UInt16()
robot_status_msg.data = 5000  #(仮)

task_status_msg = UInt16()
task_status_msg.data = 5000  #(仮)


def odomfix_callback(data):
    global get_new_odom
    global position_msg
    global orientation_msg
    global robot_status_msg
    global battery_state_msg

    position_msg = data.fix
    orientation_msg = data.orientation
    robot_status_msg.data = 5000  #(仮)
    battery_state_msg.header.stamp = data.fix.header.stamp

    get_new_odom = True

def publisher():
    global get_new_odom
    global position_msg
    global orientation_msg
    global floor_name_msg
    global battery_state_msg
    global robot_status_msg
    global task_status_msg

    rospy.init_node('mqtt_publisher', anonymous=True)

    position_sub = rospy.Subscriber('odom_fix', FixWithOrientation, odomfix_callback)

    position_pub = rospy.Publisher('wizurg/position', NavSatFix, queue_size=60)
    orientation_pub = rospy.Publisher('wizurg/pose', Quaternion, queue_size=60)
    floor_name_pub = rospy.Publisher('floor_name', String, queue_size=60)
    battery_state_pub = rospy.Publisher('battery_state', BatteryState, queue_size=60)
    robot_status_pub = rospy.Publisher('robot_status', UInt16, queue_size=60)
    task_status_pub = rospy.Publisher('task_status', UInt16, queue_size=60)
    
    rate = rospy.Rate(0.5)

    while not rospy.is_shutdown():

        if not get_new_odom:
            robot_status_msg.data = 5001  #(仮)
        
        # パブリッシュ
        rospy.loginfo(position_msg)
        position_pub.publish(position_msg)
        orientation_pub.publish(orientation_msg)
        floor_name_pub.publish(floor_name_msg)
        battery_state_pub.publish(battery_state_msg)
        robot_status_pub.publish(robot_status_msg)
        task_status_pub.publish(task_status_msg)

        get_new_odom = False

        rate.sleep()

if __name__ == '__main__':
    try:
        publisher()
    except rospy.ROSInterruptException:
        rospy.loginfo("パブリッシャーを終了します。")