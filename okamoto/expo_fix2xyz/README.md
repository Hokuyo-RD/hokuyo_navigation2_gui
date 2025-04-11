# expo_fix2xyz
緯度経度(NavSatFix)と万博プロジェクト用デカルト座標の相互変換をするパッケージです。  

## ビルド
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

## 実行手順 ( dummyデータを緯度経度に変換する )
1. dummyプログラムの実行
    ```
    roscore
    rosrun ros_tutorial dummy_xyz_pub.py
    rosrun ros_tutorial maigo_xyz_pub.py 
    rosrun ros_tutorial otosimono_xyz_pub.py
    ```
2. xyz2fixの実行
    ```
    source devel/setup.bash
    roslaunch expo_fix2xyz xyz2fix.launch
    ```
    ダミートピック`data_topic`,`maigo`,`otosimono`をそれぞれ緯度経度に変換したトピック`data_fix_topic`,`maigo_fix`,`otosimono_fix`がパブリッシュされる

## parameter
expo_software/okamoto/expo_fix2xyz/config/xyz2fix_default.yaml  
マップ原点の緯度経度と各種トピック名の設定
