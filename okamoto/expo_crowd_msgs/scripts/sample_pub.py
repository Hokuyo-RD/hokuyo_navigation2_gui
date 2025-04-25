#!/usr/bin/env python3
import rospy
import random
import math

from expo_crowd_msgs.msg import Crowd
from expo_crowd_msgs.msg import PersonArrow


def publisher():
    rospy.init_node('crowd_sample_publisher', anonymous=True)
    pub = rospy.Publisher('sample_crowd', Crowd, queue_size=10)
    rate = rospy.Rate(1)
    person_id = 0

    while not rospy.is_shutdown():
        # ランダムに1～30人のデータをパブリッシュ
        person_size = random.randint(1, 30)
        crowd_msg = Crowd()
        crowd_msg.header.stamp = rospy.Time.now()

        for i in range(person_size):
            
            # 1000人ごとにidをリセット.
            person_id = person_id % 1000

            person_msg = PersonArrow()
            person_msg.id = person_id
            person_msg.position.x = random.uniform(-100.0, 100.0)
            person_msg.position.y = random.uniform(-100.0, 100.0)
            person_msg.position.z = random.uniform(-100.0, 100.0)
            person_msg.velocity = random.randint(0, 2)

            theta = random.uniform(0,math.pi)
            person_msg.orientation.x = 0
            person_msg.orientation.y = 0
            person_msg.orientation.z = math.sin(0.5*theta)
            person_msg.orientation.w = math.cos(0.5*theta)

            crowd_msg.persons.append(person_msg)
            person_id += 1
        rate.sleep()

        # パブリッシュ
        pub.publish(crowd_msg)


if __name__ == '__main__':
    publisher()