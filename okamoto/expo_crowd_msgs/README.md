# msgs
- expo_crowd_msgs::PersonArrow  
    ある一人の固有IDと位置・速度を格納するメッセージ型です。 
    メンバ変数 | 型 | 説明  
    -| - | -
    id | int32 | 固有ID 
    position | geometry_msgs/Point | 位置
    orientation | geometry_msgs/Quaternion | 姿勢
    velocity | int32 | 速度（0,1,2の3段階を想定）
- expo_crowd_msgs::Crowd  
    PersonArrow型を配列にして混雑度情報を表すメッセージ型です。 
    メンバ変数 | 型 | 説明  
    -| - | -
    header | std_msgs/Header | タイムスタンプ等
    persons | expo_crowd_msgs/PersonArrow[] | 一人ひとりの情報

# sample
```
rosrun expo_crowd_msgs sample_pub.py
```
1秒置きに1~30人のランダムな人数のCrowdトピックをパブリッシュするサンプルプログラムです。
