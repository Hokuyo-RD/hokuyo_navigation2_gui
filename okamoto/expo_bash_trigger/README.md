# expo_bash_trigger

## ビルド
```
cd < your_workspace ( ex: ~/catkin_ws ) >
cd src
git clone https://github.com/Hokuyo-RD/expo_software.git
cd ..
catkin_make

# 実行権限の付与 
cd src/expo_software/okamoto/expo_bash_trigger/src
chmod +x bash_trigger.py
```

## 実行手順
```
rosrun expo_bash_trigger bash_trigger.py _trigger_topic:=トピック名 _bash_fullpath:=スクリプトファイルのフルパス
```