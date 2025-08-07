#!/usr/bin/env python3
import rospy
from pyproj import Transformer
import csv
import random
from geometry_msgs.msg import Vector3
from geometry_msgs.msg import Point
from expo_msgs.msg import Area
from expo_crowd_msgs.msg import LostsFix
from expo_crowd_msgs.msg import LostItemFix

def xyz_to_latlon(x, y, z):
    #x, y, z (m)を 緯度経度高度 (lat, lon, alt) に変換する

    #基準点の緯度経度高度
    lat0 = 34.648742000615925
    lon0 = 135.3864097595215
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
    pub = rospy.Publisher('lost_fix', LostsFix, queue_size=10)
    rate = rospy.Rate(1.0)


    index = 0  # データのインデックス

    rospy.loginfo("パブリッシャーを開始します。Ctrl+Cで停止してください。")
    while not rospy.is_shutdown():
        # 現在のデータを取得
        position_x,position_y,position_z=[random.randint(1, 100),random.randint(1, 100),random.randint(1, 100)] 
        # メッセージを作成
        msg = LostsFix()
        items = []
        item = LostItemFix()
        count = random.randint(1,3)
        for i in range(count):
         item.latitude,item.longitude,item.altitude = xyz_to_latlon(position_x,position_y,position_z)
            
         item.type = random.randint(0, 3)
            
         item.size = 1
            
         item.id = random.randint(1, 2)
             
         items.append(item)
        msg.lostitem = items
            
        #msg.header ="a"
            
        num = count
            # パブリッシュ
        rospy.loginfo(f"Publishing: LostsFix")
        pub.publish(msg)

        rate.sleep()

if __name__ == '__main__':
    try:
        publisher()
    except rospy.ROSInterruptException:
        rospy.loginfo("パブリッシャーを終了します。")
