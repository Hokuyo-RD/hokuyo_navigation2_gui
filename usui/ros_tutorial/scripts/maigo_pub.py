#!/usr/bin/env python3
import rospy
import csv
import random
from geometry_msgs.msg import Vector3
from geometry_msgs.msg import Point
from expo_msgs.msg import Area

def read_csv(file_path):
    """CSVファイルからデータを読み取る"""
    data = []
    with open(file_path, mode='r') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            if len(row) == 3:  # 位置（3）の3列があることを確認
                try:
                    data.append([int(row[0]), int(row[1]),int(row[2])])
                except ValueError:
                    rospy.logwarn(f"無効なデータをスキップ: {row}")
    return data

def publisher():
    rospy.init_node('csv_data_publisher', anonymous=True)
    pub = rospy.Publisher('maigo', Point, queue_size=10)
    rate = rospy.Rate(0.5)

    # CSVファイルのパス（適宜変更）
    file_path = "/home/ubuntu/catkin_ws/src/ros_tutorial/scripts/maigo.csv"
    data = read_csv(file_path)

    if not data:
        rospy.logerr("CSVファイルが空か、データが読み取れません。")
        return

    index = 0  # データのインデックス

    rospy.loginfo("パブリッシャーを開始します。Ctrl+Cで停止してください。")
    while not rospy.is_shutdown():
        # ランダムに1～5個のデータをパブリッシュ
        batch_size = random.randint(1, 5)
        for _ in range(batch_size):
            # 現在のデータを取得
            position_x,position_y,position_z= data[index]
            
            # Vector3メッセージを作成
            msg = Point()
            msg.x = position_x
            msg.y = position_y
            msg.z = position_z

            # パブリッシュ
            rospy.loginfo(f"Publishing: Position_x = {msg.x},Position_y = {msg.y},Position_z = {msg.z}")
            pub.publish(msg)

            # インデックスを更新（最後までいったら最初に戻る）
            index = (index + 1) % len(data)

        rate.sleep()

if __name__ == '__main__':
    try:
        publisher()
    except rospy.ROSInterruptException:
        rospy.loginfo("パブリッシャーを終了します。")