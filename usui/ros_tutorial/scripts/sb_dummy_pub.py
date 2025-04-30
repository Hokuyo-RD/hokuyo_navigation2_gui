#!/usr/bin/env python3
import rospy
import csv
import random
from pyproj import Transformer
from expo_msgs.msg import SB
from sensor_msgs.msg import NavSatFix
from geometry_msgs.msg import Quaternion
from sensor_msgs.msg import BatteryState 
from std_msgs.msg import UInt16
from std_msgs.msg import String

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
    lat0 = 34.64901361188646
    lon0 = 135.38359478483127
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

    position_pub = rospy.Publisher('wizurg/position', NavSatFix, queue_size=60)
    orientation_pub = rospy.Publisher('wizurg/pose', Quaternion, queue_size=60)
    floor_name_pub = rospy.Publisher('floor_name', String, queue_size=60)
    battery_state_pub = rospy.Publisher('battery_state', BatteryState, queue_size=60)
    robot_status_pub = rospy.Publisher('robot_status', UInt16, queue_size=60)
    task_status_pub = rospy.Publisher('task_status', UInt16, queue_size=60)
    
    
    
    rate = rospy.Rate(0.5)

    # CSVファイルのパス（適宜変更）
    file_path = "/home/hokuyo/catkin_ws/src/expo_software/usui/ros_tutorial/scripts/sb_data.csv"
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
            position_msg = NavSatFix()
            orientation_msg = Quaternion()
            floor_name_msg = String()
            battery_state_msg = BatteryState()
            robot_status_msg = UInt16()
            task_status_msg = UInt16()

            # msg.position.COVARIANCE_TYPE_UNKNOWN = 0
            # msg.position.COVARIANCE_TYPE_APPROXIMATED = 1
            # msg.position.COVARIANCE_TYPE_DIAGONAL_KNOWN=2
            # msg.position.COVARIANCE_TYPE_KNOWN=3

            #position_msg.header.seq = 0
            position_msg.header.stamp = rospy.Time.now()
            position_msg.header.frame_id = "test"
            # msg.position.status.STATUS_NO_FIX = -1
            # msg.position.status.STATUS_FIX = 0
            # msg.position.status.STATUS_SBAS_FIX = 1
            # msg.position.status.STATUS_GBAS_FIX = 2
            # msg.position.status.SERVICE_GPS = 1
            # msg.position.status.SERVICE_GLONASS = 2
            # msg.position.status.SERVICE_COMPASS=4
            # msg.position.status.SERVICE_GALILEO=8
            # position_msg.status.status = 0
            # msg.position.service = 0
            position_msg.latitude,position_msg.longitude,position_msg.altitude =  xyz_to_latlon(x,y,z)
            #for i in range(9):
            #    position_msg.position_covariance[i] = 0
            #position_msg.position_covariance_type = 0
            orientation_msg.x = pose_x
            orientation_msg.y = pose_y
            orientation_msg.z = pose_z
            orientation_msg.w = pose_w
            floor_name_msg.data = floor_name
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
            # battery_state_msg.header.seq = 0
            # battery_state_msg.header.stamp = 0
            # battery_state_msg.header.frame_id = "test"
            # battery_state_msg.voltage = 5
            # battery_state_msg.current = 0
            # battery_state_msg.charge = 0
            battery_state_msg.capacity  = 100
            # battery_state_msg.design_capacity = 0
            # battery_state_msg.percentage = 50
            # battery_state_msg.power_supply_status = 0
            # battery_state_msg.power_supply_health = 0
            # battery_state_msg.power_supply_technology = 0
            # battery_state_msg.present = 0
            # battery_state_msg.cell_voltage = 0
            # battery_state_msg.location = 0
            # battery_state_msg.serial_number = 0
            robot_status_msg.data = robot_status
            task_status_msg.data = task_status

            # パブリッシュ
            rospy.loginfo(f"{msg}")
            #pub.publish(msg)
            position_pub.publish(position_msg)
            orientation_pub.publish(orientation_msg)
            floor_name_pub.publish(floor_name_msg)
            battery_state_pub.publish(battery_state_msg)
            robot_status_pub.publish(robot_status_msg)
            task_status_pub.publish(task_status_msg)
    

            # インデックスを更新（最後までいったら最初に戻る）
            index = (index + 1) % len(data)

        rate.sleep()

if __name__ == '__main__':
    try:
        publisher()
    except rospy.ROSInterruptException:
        rospy.loginfo("パブリッシャーを終了します。")