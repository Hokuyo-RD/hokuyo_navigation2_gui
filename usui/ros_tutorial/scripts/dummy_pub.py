#!/usr/bin/env python3
import rospy
from pyproj import Transformer
import csv
import random
from geometry_msgs.msg import Vector3

from expo_msgs.msg import Area
from expo_msgs.msg import AreaFix

def read_csv(file_path):
    """CSVファイルからデータを読み取る"""
    data = []
    with open(file_path, mode='r') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            if len(row) == 11:  # ID, 位置（3）、サイズ、速度, 密度、向き（3）の11列があることを確認
                try:
                    data.append([int(row[0]), int(row[1]), int(row[2]),int(row[3]),int(row[4]),float(row[5]),float(row[6]),float(row[7]),float(row[8]),float(row[9]),float(row[10])])
                except ValueError:
                    rospy.logwarn(f"無効なデータをスキップ: {row}")
    return data

def xyz_to_latlon(x, y, z):
    #x, y, z (m)を 緯度経度高度 (lat, lon, alt) に変換する

    #基準点の緯度経度高度
    lat0 = 34.69176319251114
    lon0 = 135.49633723119732
    alt0 = 0

    #  基準点の緯度経度を UTM に変換
    transformer_to_utm = Transformer.from_crs("EPSG:4326", "EPSG:32654", always_xy=True)
    x0, y0 = transformer_to_utm.transform(lon0, lat0)

    #  新しいUTM座標
    x_new = x0 + x
    y_new = y0 + y

    #  UTM を 緯度経度 に変換
    transformer_to_latlon = Transformer.from_crs("EPSG:32654", "EPSG:4326", always_xy=True)
    lon, lat = transformer_to_latlon.transform(x_new, y_new)

    #  高度を加算
    alt = alt0 + z

    return lat, lon, alt

def publisher():
    rospy.init_node('csv_data_publisher', anonymous=True)
    pub = rospy.Publisher('data_topic', AreaFix, queue_size=30)
    rate = rospy.Rate(0.5)

    # CSVファイルのパス（適宜変更）
    file_path = "/home/ubuntu/catkin_ws/src/ros_tutorial/scripts/data.csv"
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
            id_value, position_x,position_y,position_z,size,velocity_value, density_value,pose_x,pose_y,pose_z,pose_w = data[index]
            
            # Vector3メッセージを作成
            msg = AreaFix()
            msg.id = id_value
            msg.latitude,msg.longitude,msg.altitude = xyz_to_latlon(position_x,position_y,position_z)
            msg.size = size
            msg.velocity = velocity_value
            msg.density = density_value
            msg.pose.x = pose_x
            msg.pose.y = pose_y
            msg.pose.z = pose_z
            msg.pose.w = pose_w

            # パブリッシュ
            rospy.loginfo(f"Publishing: ID={msg.id},Position_x = {msg.latitude},Position_y = {msg.longitude},Position_z = {msg.altitude},Size = {msg.size}, Velocity={msg.velocity}, Density={msg.density},Pose_x = {msg.pose.x},Pose_y = {msg.pose.y},Pose_z = {msg.pose.z},,Pose_w = {msg.pose.w}")
            pub.publish(msg)

            # インデックスを更新（最後までいったら最初に戻る）
            index = (index + 1) % len(data)

        rate.sleep()

if __name__ == '__main__':
    try:
        publisher()
    except rospy.ROSInterruptException:
        rospy.loginfo("パブリッシャーを終了します。")