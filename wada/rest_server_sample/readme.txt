REST APIにてアクセスできるサーバを動かすサンプル
特定の電文に対応してホスト側でスクリプトを起動する

------------------------

・python3を使用
flaskをインストール

sudo apt install python-pip3
pip install flask

→ "error: externally-managed-environment" というエラーが出る
仮想環境にてインストールや実行をしてね、とのこと
（参考）https://qiita.com/toiee_kame/items/c3781abb53f385dae4f9


sudo apt install python3.12-venv
python3 -m venv .
source ~/python/bin/activate
pip install flask

-> OK

------------------------

プログラム上でポート5000にアクセスするので開ける

sudo ufw enable
sudo ufw allow 5000/tcp

------------------------

pythonプログラムを起動

python3 server.py


ブラウザからアクセス

http://192.168.137.100:5000/trigger
 -> "Run Script" ボタンを押す
 
ホスト側にて"sample_script_sh"が起動し、
"script_log.txt" が生成される


