#人の情報を格納する.

int32 id                              # 固有番号.
geometry_msgs/Point position          # XY座標.
geometry_msgs/Quaternion orientation  # 向き.
int32 velocity                        # 速度（3段階）.
int32 state                           # 状態 (0:通常、1:見失い)

