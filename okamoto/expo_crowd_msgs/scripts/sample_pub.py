#!/usr/bin/env python3
import rospy
import random
import math

from expo.msg import CrowdEX
from expo.msg import PersonArrowEX
from expo_crowd_msgs.msg import Losts
from expo_crowd_msgs.msg import LostItem


def publisher():
    rospy.init_node('crowd_sample_publisher', anonymous=True)
    pub = rospy.Publisher('crowd', CrowdEX, queue_size=10)
    losts_pub = rospy.Publisher('losts', Losts, queue_size=10)
    rate = rospy.Rate(1)
    person_id = 0
    losts_id = 0

    while not rospy.is_shutdown():
        # ランダムに1～30人の人と、1~5個の落とし物データをパブリッシュ
        person_size = random.randint(1, 30)
        crowd_msg = CrowdEX()
        crowd_msg.header.stamp = rospy.Time.now()
        crowd_msg.header.frame_id = "map"

        losts_size = random.randint(1,5)
        losts_msg = Losts()
        losts_msg.num = losts_size
        losts_msg.header.stamp = rospy.Time.now()
        losts_msg.header.frame_id = "map"

        for i in range(person_size):
            
            # 1000人ごとにidをリセット.
            person_id = person_id % 1000

            person_msg = PersonArrowEX()
            person_msg.id = person_id
            person_msg.position.x = random.uniform(-100.0, 100.0)
            person_msg.position.y = random.uniform(-100.0, 100.0)
            person_msg.position.z = random.uniform(-100.0, 100.0)
            person_msg.velocity = random.randint(0, 2)
            person_msg.state = random.randint(0,1)

            theta = random.uniform(0,math.pi)
            person_msg.orientation.x = 0
            person_msg.orientation.y = 0
            person_msg.orientation.z = math.sin(0.5*theta)
            person_msg.orientation.w = math.cos(0.5*theta)

            crowd_msg.persons.append(person_msg)
            person_id += 1

        for i in range(losts_size):
            # 10個ごとにidをリセット.
            losts_id = losts_id % 3

            lostitem_msg = LostItem()
            lostitem_msg.id = losts_id
            lostitem_msg.position.x = random.uniform(-10.0, 10.0)
            lostitem_msg.position.y = random.uniform(-10.0, 10.0)
            lostitem_msg.position.z = random.uniform(-10.0, 10.0)
            lostitem_msg.size = random.randint(0,2)
            lostitem_msg.type = random.randint(0,2)

            losts_msg.lostitem.append(lostitem_msg)
            losts_id += 1

        rate.sleep()

        # パブリッシュ
        pub.publish(crowd_msg)
        losts_pub.publish(losts_msg)
        print("publish!")


if __name__ == '__main__':
    publisher()