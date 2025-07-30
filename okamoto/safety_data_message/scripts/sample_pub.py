#!/usr/bin/env python3
import rospy
import random
import math

from safety_data_message.msg import SafetyData


def publisher():
    count = 0
    safe_count = 100
    warning_count = 50
    danger_count = 50

    rospy.init_node('safety_sample_publisher', anonymous=True)
    pub1 = rospy.Publisher('uam1',SafetyData , queue_size=10)
    pub2 = rospy.Publisher('uam2',SafetyData , queue_size=10)
    rate = rospy.Rate(20)

    # count < safe_count の間、エラー無し.
    # safe_count < count < safe_count + warning_count の間、warningあり.
    # safe_count + warning_count < count < safe_count + warning_count + danger_count の間、ossdあり.
    while not rospy.is_shutdown():
        count += 1
        if count > safe_count + warning_count + danger_count:
            count = 0

        uam1_msg = SafetyData()
        uam2_msg = SafetyData()
        uam1_msg.header.stamp = rospy.Time.now()
        uam2_msg.header.stamp = rospy.Time.now()

        if count < safe_count:
            uam1_msg.ossd_1_status = 0
            uam2_msg.ossd_1_status = 0
            uam1_msg.warning_1_status = 0
            uam2_msg.warning_1_status = 0
        elif count < safe_count + warning_count :
            uam1_msg.ossd_1_status = 0
            uam2_msg.ossd_1_status = 0
            uam1_msg.warning_1_status = 1
            uam2_msg.warning_1_status = 1
        else:
            uam1_msg.ossd_1_status = 1
            uam2_msg.ossd_1_status = 1
            uam1_msg.warning_1_status = 1
            uam2_msg.warning_1_status = 1
        
        rate.sleep()

        # パブリッシュ
        pub1.publish(uam1_msg)
        pub2.publish(uam2_msg)
        print("publish!")


if __name__ == '__main__':
    publisher()
