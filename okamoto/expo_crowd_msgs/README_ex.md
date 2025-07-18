# msgs(混雑度)
- expo_crowd_msgs::PersonArrowEX  
    ある一人の固有IDと位置・速度を格納するメッセージ型です。 
    メンバ変数 | 型 | 説明  
    -| - | -
    id | int32 | 固有ID 
    position | geometry_msgs/Point | 位置
    orientation | geometry_msgs/Quaternion | 姿勢
    velocity | int32 | 速度（0,1,2の3段階を想定）
    state | int32 | 状態 (0:通常、1:見失い)
    
- expo_crowd_msgs::CrowdEX
    PersonArrowEX型を配列にして混雑度情報を表すメッセージ型です。 
    メンバ変数 | 型 | 説明  
    -| - | -
    header | std_msgs/Header | タイムスタンプ等
    persons | expo_crowd_msgs/PersonArrowEX[] | 一人ひとりの情報
# msgs(混雑度_緯度経度)
- expo_crowd_msgs::PersonArrowFixEX
    PersonArrowEX型の緯度経度版です。 
    メンバ変数 | 型 | 説明  
    -| - | -
    id | int32 | 固有ID 
    latitude | float64 | 緯度[度] 
    longitude | float64 | 経度[度] 
    altitude | float64 | 高度[m]
    orientation | geometry_msgs/Quaternion | 姿勢
    velocity | int32 | 速度（0,1,2の3段階を想定）
    state | int32 | 状態 (0:通常、1:見失い)
- expo_crowd_msgs::CrowdFixEX
    CrowdEX型の緯度経度版です。 
    メンバ変数 | 型 | 説明  
    -| - | -
    header | std_msgs/Header | タイムスタンプ等
    persons | expo_crowd_msgs/PersonArrowFixEX[] | 一人ひとりの情報

# msgs(落とし物)
- expo_crowd_msgs::LostItem  
    落とし物の位置と種類を格納するメッセージ型です。 
    メンバ変数 | 型 | 説明  
    -| - | -
    id | int32 | 固有ID ※ 空でもよい
    position | geometry_msgs/Point | 位置
    size | int32 | 物体の大きさ
    type | int32 | 認識した種類
- expo_crowd_msgs::Losts
    LostItem型を配列にして落とし物情報を表すメッセージ型です。 
    メンバ変数 | 型 | 説明  
    -| - | -
    header | std_msgs/Header | タイムスタンプ等
    num | int32 | 検出した数
    lostitem | expo_crowd_msgs/LostItem[] | 落とし物1つの情報

# msgs(落とし物_緯度経度)
- expo_crowd_msgs::LostItemFix
    LostItem型の緯度経度版です。  
    メンバ変数 | 型 | 説明  
    -| - | -
    id | int32 | 固有ID ※ 空でもよい
    latitude | float64 | 緯度[度] 
    longitude | float64 | 経度[度] 
    altitude | float64 | 高度[m]
    size | int32 | 物体の大きさ
    type | int32 | 認識した種類
- expo_crowd_msgs::Losts
    LostItemFix型を配列にして落とし物情報を表すメッセージ型です。 
    メンバ変数 | 型 | 説明  
    -| - | -
    header | std_msgs/Header | タイムスタンプ等
    num | int32 | 検出した数
    lostitem | expo_crowd_msgs/LostItemFix[] | 落とし物1つの情報