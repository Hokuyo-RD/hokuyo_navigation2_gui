# 万博プロジェクト　岡本

## expo_fix_msgs
expo_msgs の緯度経度バージョンです。

### msgs
- expo_fix_msgs::AreaFix  
    メンバ変数 | 型 | 説明  
    -| - | -
    id | [int32] | エリアID 
    latitude | [float64] | 緯度[度] 
    longitude | [float64] | 経度[度] 
    altitude | [float64] | 高度[m] 
    size | [int32] | エリアサイズ 
    velocity | [float32] | 速度 
    density | [float32] | 密度 
    pose | [geometry_msgs/Quaternion] | 姿勢 
- expo_fix_msgs::PersonFix
    メンバ変数 | 型 | 説明  
    -| - | -
    latitude | [float64] | 緯度[度] 
    longitude | [float64] | 経度[度] 
    altitude | [float64] | 高度[m] 
    orientation | [geometry_msgs/Quaternion] | 姿勢 
    velocity | [float32] | 速度 

## expo_fix2xyz
fixトピックから万博プロジェクト用の規定デカルト座標系へ変換するパッケージです。  
原点の緯度経度を指定して、fixトピックをxyzに変換します。

### ビルド
```
# 依存ライブラリ
sudo apt-get install libsqlite3-dev sqlite3
wget https://download.osgeo.org/proj/proj-9.4.1.tar.gz
cd proj-9.4.1
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --build . --target install

# ROSパッケージ
cd < your_workspace ( ex: ~/catkin_ws ) >
cd src
git clone https://github.com/Hokuyo-RD/expo_software.git
cd ..
catkin_make
```

### 実行手順
1. parameterファイルを編集する ( 詳細は後述の parameter を参照 )  
    - expo_software/okamoto/expo_fix2xyz/config/fix2xyz_default.yaml
2. launch実行
    ```
    source devel/setup.bash
    roslaunch expo_fix2xyz fix2xyz.launch
    ```

### parameter

#### 原点の設定
- `origin_pose` (default: 34.69176319251114,135.49633723119732,0)
    - マップ原点の( 緯度[度] , 経度[度] , 高度[m] )
- `origin_quat` (default: 0.0,0.0,0.0,1.0)
    - 緯度経度からみたマップの姿勢。基本変更不要。

#### Area => AreaFix
- `sub_area_topic` (default: "data_topic")
    - AreaFix型に変換したいareaトピック名  
    [expo_msgs::Area]
- `pub_area_fix_topic` (default: "area_fix/from_area")
    - `sub_area_topic`を変換してpublishするarea_fixトピック名  
    [expo_fix_msgs::AreaFix]

#### AreaFix => Area
- `sub_area_fix_topic` (default: "area_fix")
    - Area型に変換したいarea_fixトピック名  
    [expo_fix_msgs::AreaFix]
- `pub_area_topic` (default: "area/from_fix")
    - `sub_area_fix_topic`を変換してpublishするareaトピック名  
    [expo_msgs::Area]

#### Person => PersonFix
- `sub_person_topic` (default: "person")
    - PersonFix型に変換したいpersonトピック名  
    [expo_msgs::Person]
- `pub_person_fix_topic`(default: "person_fix/from_person")
    - `sub_person_topic`を変換してpublishするperson_fixトピック名  
    [expo_fix_msgs::PersonFix]

#### PersonFix => Person
- `sub_person_fix_topic` (default: "person_fix")
    - Person型に変換したいperson_fixトピック名  
    [expo_fix_msgs::PersonFix]
- `pub_person_topic` (default: "person/from_fix")
    - `sub_person_fix_topic`を変換してpublishするpersonトピック名  
    [expo_msgs::Person]


#### NavSatFix => Odometry
- `sub_fix_topic1` (default: "fix1")
    - Odometry型に変換したいfixトピック名  
    [sensor_msgs::NavSatFix]
- `pub_odom_topic1` (default: "odometry/from_fix1")
    - `sub_fix_topic1`を変換してpublishするodometryトピック名  
    [nav_msgs::Odometry]

#### Odometry => NavSatFix
- `sub_odom_topic1` (default: "odom1")
    - NavSatFix型に変換したいodometryトピック名  
    [nav_msgs::Odometry]
- `pub_fix_topic1` (default: "fix/from_odom1")
    - `sub_odom_topic1`を変換してpublishするfixトピック名  
    [sensor_Msgs::NavSatFix]

#### NavSatFix => PoseStamped
- `sub_fix_topic2` (default: "fix2")
    - PoseStamped型に変換したいfixトピック名  
    [sensor_msgs::NavSatFix]
- `pub_pose_topic2` (default: "pose/from_fix2")
    - `sub_fix_topic2`を変換してpublishするPoseStampedトピック名  
    [geometry_msgs::PoseStamped]

#### PoseStamped　=> NavSatFix
- `sub_pose_topic2` (default: "pose2")
    - NavSatFix型に変換したいPoseStampedトピック名  
    [geometry_msgs::PoseStamped]
- `pub_fix_topic2` (default: "fix/from_pose2")
    - `sub_pose_topic2`を変換してpublishするfixトピック名  
    [sensor_msgs::NavSatFix]

#### NavSatFix => PoseWithCovarianceStamped
- `sub_fix_topic3` (default: "fix3")
    - PoseWithCovarianceStamped型に変換したいfixトピック名  
    [sensor_msgs::NavSatFix]
- `pub_posecov_topic3` (default: "posecov/from_fix3")
    - `sub_fix_topic3`を変換してpublishするPoseWithCovarianceStampedトピック名  
    [geometry_msgs::PoseWithCovarianceStamped]


#### PoseWithCovarianceStamped => NavSatFix
- `sub_posecov_topic3` (default: "posecov3")
    - NavSatFix型に変換したいPoseWithCovarianceStampedトピック名  
    [geometry_msgs::PoseWithCovarianceStamped]
- `pub_fix_topic3` (default: "fix/from_posecov3")
    - `sub_posecov_topic3`を変換してpublishするfixトピック名  
    [sensor_msgs::NavSatFix]

#### その他のオプション
- `rot_cov` (default: 1000000000.0)
    - fixからOdometryやPoseWithCovarianceStampedに変換する際に必要となる、回転に関する共分散行列の対角成分.
- `make_angle_from_movement` (default: true)
    - trueにすると、fixからOdometry,PoseWithCovarianceStamped,PoseStampedに変換する際に、前回のfixとの差から姿勢を推定する.
