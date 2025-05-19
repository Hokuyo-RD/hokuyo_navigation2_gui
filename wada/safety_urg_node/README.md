# safety_urg_node
北陽電機の安全センサと通信するノード

## ビルド
```
# 依存ライブラリ
sudo apt-get install libsqlite3-dev sqlite3
wget https://download.osgeo.org/proj/proj-9.4.1.tar.gz
tar -zxvf proj-9.4.1.tar.gz
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

