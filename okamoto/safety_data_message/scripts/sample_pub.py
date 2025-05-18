#!/usr/bin/env python3
import rospy
import random
import math

from safety_data_message.msg import SafetyData


def publisher():
    rospy.init_node('safety_sample_publisher', anonymous=True)
    pub1 = rospy.Publisher('uam1',SafetyData , queue_size=10)
    pub2 = rospy.Publisher('uam2',SafetyData , queue_size=10)
    rate = rospy.Rate(10)

    while not rospy.is_shutdown():
        uam1_msg = SafetyData()
        uam2_msg = SafetyData()
        uam1_msg.header.stamp = rospy.Time.now()
        uam2_msg.header.stamp = rospy.Time.now()
        
        random_int = random.randint(0, 9)
        if random_int == 0:
            uam1_msg.ossd_1_status = 0
        else:
            uam1_msg.ossd_1_status = 1
        if random_int <= 2:
            uam1_msg.warning_1_status = 0
        else:
            uam1_msg.warning_1_status = 1
            
        if random_int == 9:
            uam2_msg.ossd_1_status = 0
        else:
            uam2_msg.ossd_1_status = 1
        if random_int >= 7:
            uam2_msg.warning_1_status = 0
        else:
            uam2_msg.warning_1_status = 1
            
        rate.sleep()

        # パブリッシュ
        pub1.publish(uam1_msg)
        pub2.publish(uam2_msg)
        print("publish!")


if __name__ == '__main__':
    publisher()
