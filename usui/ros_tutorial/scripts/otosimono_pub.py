#!/usr/bin/env python3
import rospy
from pyproj import Transformer
import csv
import random
from geometry_msgs.msg import Vector3
from geometry_msgs.msg import Point
from expo_msgs.msg import Area
from expo_msgs.msg import PositionFix

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
    pub = rospy.Publisher('otosimono', Point, queue_size=10)
    rate = rospy.Rate(0.5)

    # CSVファイルのパス（適宜変更）
    file_path = "/home/ubuntu/catkin_ws/src/ros_tutorial/scripts/otosimono.csv"
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
            
            # メッセージを作成
            msg = Point()
            msg.x,msg.y,msg.z = xyz_to_latlon(position_x,position_y,position_z)

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