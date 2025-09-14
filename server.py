#!/usr/bin/env python3

from flask import Flask, request, render_template, redirect, jsonify
from flask_sockets import Sockets
import asyncio
import websockets
import os
import subprocess
from threading import Thread
from gevent import pywsgi
from geventwebsocket.handler import WebSocketHandler
from geventwebsocket.websocket import WebSocket


# 環境変数をチェックして、実行するスクリプトのベースパスを決定
if 'DOCKER_CONTAINER' in os.environ:
    # Dockerコンテナ内でのパス
    BASE_PATH = "$HOME/colcon_ws/src/hokuyo_navigation2/scripts/"
else:
    # ホストOS上でのパス
    BASE_PATH = "$HOME/colcon_ws/src/hokuyo_navigation2/scripts/" # ホストOS上の正しいパスに置き換えてください

app = Flask(__name__)
sockets = Sockets(app)

# グローバル変数で現在のモードを管理
current_mode = "stopped"

ROSBRIDGE_URI = "ws://localhost:9090"

async def forward(ws, target):
    try:
        while True:
            try:
                message = await ws.recv() if isinstance(ws, websockets.legacy.client.WebSocketClientProtocol) else ws.receive()
                if message is None and isinstance(ws, WebSocket):
                    print(f"Forwarder (gevent): Received None, connection closed: {ws}")
                    break
                await target.send(message)
            except (websockets.exceptions.ConnectionClosedOK, websockets.exceptions.ConnectionClosedError) as e:
                print(f"Forwarder: Connection closed: {ws}, error: {e}")
                break
            except Exception as e:
                print(f"Forwarder: Error during receive/send on {ws}: {e}, type: {e.__class__.__name__}")
                break
    finally:
        print(f"Forwarder: Exiting for {ws}")

async def proxy(websocket):
    ros_ws = None
    try:
        ros_ws = await websockets.connect(ROSBRIDGE_URI)
        print(f"Proxy: Connected to ROSBridge: {ROSBRIDGE_URI}")
        await asyncio.gather(
            forward(websocket, ros_ws),
            forward(ros_ws, websocket),
        )
    except ConnectionRefusedError:
        print(f"Proxy: Error: Could not connect to ROSBridge at {ROSBRIDGE_URI}. Make sure it's running.")
    except Exception as e:
        print(f"Proxy: WebSocket proxy error: {e}, type: {e.__class__.__name__}")
    finally:
        print("Proxy: WebSocket proxy connection closed.")
        if ros_ws and not ros_ws.closed:
            await ros_ws.close()

@sockets.route('/ws')
def websocket_handler(ws):
    asyncio.run(proxy(ws))

# 新しいエンドポイントを追加
@app.route('/get_mode')
def get_mode():
    global current_mode
    return jsonify(mode=current_mode)

@app.route('/indoor_run')
def indoor_run():
    return render_template('indoor_run.html')

@app.route('/indoor_run_popup')
def indoor_run_popup():
    return render_template('indoor_run_popup.html')

@app.route('/outdoor_run_popup')
def outdoor_run_popup():
    return render_template('outdoor_run_popup.html')

@app.route('/stop')
def stop_run():
    return render_template('stop.html')

@app.route('/demo_executed')
def demo_run():
    return render_template('demo_executed.html', message="デモモードに切り替わりました。Viewerでジョイスティックを使ってデモをしてください。")

def run_subprocess(command_list):
    subprocess.run(command_list)

@app.route('/program_executed')
def program_executed():
    return render_template('program_executed.html', message="自律走行が開始されました。周囲の安全に気をつけて下さい。")

@app.route('/wizurg', methods=['GET', 'POST'])
def trigger_script():
    global current_mode
    if request.method == 'GET':
        return render_template('index.html')
    elif request.method == 'POST':
        command = request.form.get("command") or request.get_json().get("command")
        if command == "execute_indoor_run":
            check1 = request.form.get("check1")
            check2 = request.form.get("check2")
            check3 = request.form.get("check3")
            if check1 == 'checked' and check2 == 'checked' and check3 == 'checked':
                script_path = os.path.join(BASE_PATH, "expo_in")
                Thread(target=run_subprocess, args=([script_path],)).start()
                current_mode = "running"
                return redirect('/program_executed')
            else:
                return render_template('indoor_run_popup.html', error="すべてのチェック項目にチェックを入れてください。")
        elif command == "execute_outdoor_run":
            check1_outdoor = request.form.get("check1_outdoor")
            check2_outdoor = request.form.get("check2_outdoor")
            check3_outdoor = request.form.get("check3_outdoor")
            if check1_outdoor == 'checked' and check2_outdoor == 'checked' and check3_outdoor == 'checked':
                script_path = os.path.join(BASE_PATH, "nav_single_map.sh")
                Thread(target=run_subprocess, args=([script_path],)).start()
                current_mode = "running"
                return redirect('/program_executed')
            else:
                return render_template('outdoor_run_popup.html', error="すべてのチェック項目にチェックを入れてください。")
        elif command == "stop":
            script_path = os.path.join(BASE_PATH, "web_kill_all_rosnode.sh")
            Thread(target=run_subprocess, args=([script_path],)).start()
            current_mode = "stopped"
            return render_template('stop.html')
        elif command == "demo":
            script_path = os.path.join(BASE_PATH, "get_data.sh")
            Thread(target=run_subprocess, args=([script_path],)).start()
            current_mode = "demo"
            return redirect('/demo_executed')
        elif command == "map":
            return render_template('map.html')
        else:
            print(command)
            return "Unknown command", 400

if __name__ == '__main__':
    from gevent import pywsgi
    from geventwebsocket.handler import WebSocketHandler
    from geventwebsocket.websocket import WebSocket
    from flask import redirect

    server = pywsgi.WSGIServer(('0.0.0.0', 5050), app, handler_class=WebSocketHandler)
    print("WebSocket Proxy server started at ws://0.0.0.0:5050/ws")
    server.serve_forever()