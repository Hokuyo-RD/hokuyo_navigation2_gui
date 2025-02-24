# 万博プロジェクト　岡本

## expo_latlon2xyz (作業中)
fixトピックから万博プロジェクト用の規定デカルト座標系へ変換するパッケージ  
指定した緯度経度を原点にして、サブスクライブしたfixトピックをxy座標に変換する

### 依存ライブラリ
```
sudo apt-get install libsqlite3-dev sqlite3

wget https://download.osgeo.org/proj/proj-9.4.1.tar.gz
cd proj-9.4.1
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --build . --target install
```