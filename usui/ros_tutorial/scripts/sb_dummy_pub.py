#!/usr/bin/env python3
import rospy
import csv
import random
from pyproj import Transformer
from expo_msgs.msg import SB

def read_csv(file_path):
    """CSVファイルからデータを読み取る"""
    data = []
    with open(file_path, mode='r') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            if len(row) == 10:  # x,y,z,pose_x,pose_y,pose_z,pose_w,floor_name,robot_status,task_statusの10列があることを確認
                try:
                    data.append([int(row[0]), int(row[1]), int(row[2]),float(row[3]),float(row[4]),float(row[5]),float(row[6]),(row[7]),int(row[8]),int(row[9])])
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
    pub = rospy.Publisher('data_topic', SB, queue_size=60)
    rate = rospy.Rate(0.5)

    # CSVファイルのパス（適宜変更）
    file_path = "/home/ubuntu/catkin_ws/src/ros_tutorial/scripts/sb_data.csv"
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
            x,y,z,pose_x,pose_y,pose_z,pose_w,floor_name,robot_status,task_status = data[index]
            
            # Vector3メッセージを作成
            msg = SB()
            # msg.position.COVARIANCE_TYPE_UNKNOWN = 0
            # msg.position.COVARIANCE_TYPE_APPROXIMATED = 1
            # msg.position.COVARIANCE_TYPE_DIAGONAL_KNOWN=2
            # msg.position.COVARIANCE_TYPE_KNOWN=3
            msg.position.header.seq = 0
            msg.position.header.stamp = 0
            msg.position.header.frame_id = "test"
            # msg.position.status.STATUS_NO_FIX = -1
            # msg.position.status.STATUS_FIX = 0
            # msg.position.status.STATUS_SBAS_FIX = 1
            # msg.position.status.STATUS_GBAS_FIX = 2
            # msg.position.status.SERVICE_GPS = 1
            # msg.position.status.SERVICE_GLONASS = 2
            # msg.position.status.SERVICE_COMPASS=4
            # msg.position.status.SERVICE_GALILEO=8
            msg.position.status = 0
            # msg.position.service = 0
            msg.position.latitude,msg.position.longitude,msg.position.altitude =  xyz_to_latlon(x,y,z)
            for i in range(9):
                msg.position.position_covariance[i] = 0
            msg.position.position_covariance_type = 0
            msg.pose.x = pose_x
            msg.pose.y = pose_y
            msg.pose.z = pose_z
            msg.pose.w = pose_w
            msg.floor_name = floor_name
            # msg.battery_state.POWER_SUPPLY_STATUS_UNKNOWN=0
            # msg.battery_state.POWER_SUPPLY_STATUS_CHARGING=1
            # msg.battery_state.POWER_SUPPLY_STATUS_DISCHARGING=2
            # msg.battery_state.POWER_SUPPLY_STATUS_NOT_CHARGING=3
            # msg.battery_state.POWER_SUPPLY_STATUS_FULL=4
            # msg.battery_state.POWER_SUPPLY_HEALTH_UNKNOWN=0
            # msg.battery_state.POWER_SUPPLY_HEALTH_GOOD=1
            # msg.battery_state.POWER_SUPPLY_HEALTH_OVERHEAT=2
            # msg.battery_state.POWER_SUPPLY_HEALTH_DEAD=3
            # msg.battery_state.POWER_SUPPLY_HEALTH_OVERVOLTAGE=4
            # msg.battery_state.POWER_SUPPLY_HEALTH_UNSPEC_FAILURE=5
            # msg.battery_state.POWER_SUPPLY_HEALTH_COLD=6
            # msg.battery_state.POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE=7
            # msg.battery_state.POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE=8
            # msg.battery_state.POWER_SUPPLY_TECHNOLOGY_UNKNOWN=0
            # msg.battery_state.POWER_SUPPLY_TECHNOLOGY_NIMH=1
            # msg.battery_state.POWER_SUPPLY_TECHNOLOGY_LION=2
            # msg.battery_state.POWER_SUPPLY_TECHNOLOGY_LIPO=3
            # msg.battery_state.POWER_SUPPLY_TECHNOLOGY_LIFE=4
            # msg.battery_state.POWER_SUPPLY_TECHNOLOGY_NICD=5
            # msg.battery_state.POWER_SUPPLY_TECHNOLOGY_LIMN=6
            msg.battery_state.header.seq = 0
            msg.battery_state.header.stamp = 0
            msg.battery_state.header.frame_id = "test"
            msg.battery_state.voltage = 5
            msg.battery_state.current = 0
            msg.battery_state.charge = 0
            msg.battery_state.capacity  = 100
            msg.battery_state.design_capacity = 0
            msg.battery_state.percentage = 50
            msg.battery_state.power_supply_status = 0
            msg.battery_state.power_supply_health = 0
            msg.battery_state.power_supply_technology = 0
            msg.battery_state.present = 0
            msg.battery_state.cell_voltage = 0
            msg.battery_state.location = 0
            msg.battery_state.serial_number = 0
            msg.robot_status = robot_status
            msg.task_status = task_status

            # パブリッシュ
            rospy.loginfo(f"{msg}")
            pub.publish(msg)

            # インデックスを更新（最後までいったら最初に戻る）
            index = (index + 1) % len(data)

        rate.sleep()

if __name__ == '__main__':
    try:
        publisher()
    except rospy.ROSInterruptException:
        rospy.loginfo("パブリッシャーを終了します。")