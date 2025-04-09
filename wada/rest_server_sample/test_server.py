#!/usr/bin/env python3

from flask import Flask, request, jsonify
import subprocess

app = Flask(__name__)

@app.route('/trigger', methods=['GET', 'POST'])
def trigger_script():
    if request.method == 'GET':
        return '''
            <form method="post" action="/trigger">
                <button type="submit" name="command" value="run" style="width:200px;height:100px">自律移動開始</button>
            </form>
            <form method="post" action="/trigger">
                <button type="submit" name="command" value="stop" style="width:200px;height:100px">プログラム停止</button>
            </form>
            <form method="post" action="/trigger">
                <button type="submit" name="command" value="tools" style="width:200px;height:100px">設定ツール起動</button>
            </form>
            <a href="http://192.168.137.28:8085/ros_map_demo/">マップ</a>
            '''
    elif request.method == 'POST':
        command = request.form.get("command") or request.get_json().get("command")
        if command == "run":
            subprocess.run(["/home/hokuyo/catkin_ws/src/expo_wizurg/scripts/expo_nav.sh"])
            return '''
                    <h1>WizURG操作画面</h1>
                    <p>自律移動プログラムが実行されました。</p>
                '''
        elif command == "stop":
            subprocess.run(["/home/hokuyo/catkin_ws/src/expo_wizurg/scripts/web_kill_all_rosnode.sh"])
            return '''
                    <h1>WizURG操作画面</h1>
                    <p>プログラムを終了します。</p>
                '''
        elif command == "tools":
            subprocess.run(["/home/hokuyo/catkin_ws/src/expo_wizurg/scripts/expo_start.sh"])
            return '''
                    <h1>WizURG操作画面</h1>
                    <p>設定ツールを起動します。</p>
                '''
        else:
            print(command)
            return "Unknown command", 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)