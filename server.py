#!/usr/bin/env python3

from flask import Flask, request, render_template, redirect, jsonify, url_for, flash
from flask_sockets import Sockets
import asyncio
import websockets
import os
import subprocess
from threading import Thread
from gevent import pywsgi
from geventwebsocket.handler import WebSocketHandler
from geventwebsocket.websocket import WebSocket
import shlex # コマンドインジェクションを防ぐために使用

# 環境変数をチェックして、実行するスクリプトのベースパスを決定
if 'DOCKER_CONTAINER' in os.environ:
    # Dockerコンテナ内でのパス
    BASE_PATH = "/home/colcon_ws/src/hokuyo_navigation2/scripts/"
    # ファイルブラウザがアクセスを許可されている最上位のディレクトリ
    ROOT_DIR = "/home/colcon_ws/src/hokuyo_navigation2"
else:
    # ホストOS上でのパス
    BASE_PATH = "/home/hokuyo/colcon_ws/src/hokuyo_navigation2/scripts/"
    # ファイルブラウザがアクセスを許可されている最上位のディレクトリ
    ROOT_DIR = "/home/hokuyo/colcon_ws/src/hokuyo_navigation2"

app = Flask(__name__)
sockets = Sockets(app)

# セッションメッセージを有効にするための設定
app.secret_key = 'your_secret_key_here' # 任意の秘密鍵を設定してください

# グローバル変数で現在のモードを管理
current_mode = "stopped"

ROSBRIDGE_URI = "ws://localhost:9090"

async def forward(ws, target):
    try:
        while True:
            try:
                # gevent WebSocket と websockets.client の recv/receive の違いに対応
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
        # asyncio.gather を使うことで、双方向の転送を同時に処理できます
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

# 既存のエンドポイント
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

@app.route('/mapping_executed')
def mapping_run():
    # マッピング開始後のメッセージ
    return render_template('demo_executed.html', message="マッピングが開始されました。安全に注意し、周囲を走行してください。")

@app.route('/mapping_popup')
def mapping_run_popup():
    return render_template('mapping_popup.html')

@app.route('/demo_executed')
def demo_run():
    return render_template('demo_executed.html', message="手動操作モードに切り替わりました。Viewerでジョイスティックを使ってデモをしてください。")

def run_subprocess(command_list):
    """別スレッドでサブプロセスを実行するための関数"""
    subprocess.run(command_list)

@app.route('/program_executed')
def program_executed():
    return render_template('program_executed.html', message="自律走行が開始されました。周囲の安全に気をつけて下さい。")

# ----------------------------------------------------
## ファイルブラウザ機能 (階層アクセス対応)
# ----------------------------------------------------

@app.route('/browse', defaults={'path': ''})
@app.route('/browse/<path:path>')
def browse(path):
    """ディレクトリ内のファイルとディレクトリを一覧表示する（再帰アクセス対応）"""
    global ROOT_DIR
    
    # ユーザーが要求したパスをルートディレクトリに結合
    full_path = os.path.join(ROOT_DIR, path)
    
    # パスが ROOT_DIR の外に出ていないかセキュリティチェック
    absolute_root_dir = os.path.abspath(ROOT_DIR)
    absolute_full_path = os.path.abspath(full_path)
    
    if not absolute_full_path.startswith(absolute_root_dir):
        flash("セキュリティ上の理由により、このディレクトリにはアクセスできません。", "error")
        return redirect(url_for('browse'))
    
    # フォルダが存在するかどうかをチェック
    if not os.path.isdir(full_path):
        flash(f"ディレクトリが見つかりません: {path}", "error")
        return redirect(url_for('browse', path=os.path.dirname(path)))

    try:
        items = os.listdir(full_path)
        
        # ディレクトリとファイルの区別をつける
        files = [item for item in items if os.path.isfile(os.path.join(full_path, item))]
        dirs = [item for item in items if os.path.isdir(os.path.join(full_path, item))]
        
        # 親ディレクトリのパスを計算 (ROOT_DIR ではない場合のみ)
        parent_path = os.path.dirname(path) if path else None

        # browse.htmlに渡す
        return render_template('browse.html', 
                               files=files, 
                               dirs=dirs, 
                               current_path=path, # URLに使う相対パス
                               current_dir_name=os.path.basename(full_path) if path else absolute_root_dir, # ルートの場合は絶対パスを表示
                               root_dir=ROOT_DIR,
                               parent_path=parent_path)

    except FileNotFoundError:
        flash("指定されたディレクトリが見つかりません。", "error")
        return redirect(url_for('browse'))
    except PermissionError:
        flash("ディレクトリへのアクセス権限がありません。", "error")
        return redirect(url_for('browse'))


@app.route('/handle_file_path', methods=['POST'])
def handle_file_path():
    """クライアントから送られたファイルパスをsubprocessに渡す"""
    global ROOT_DIR
    file_path = request.form.get('file_path') # 相対パスが格納されている
    
    if file_path:
        # パスが ROOT_DIR の外に出ていないかセキュリティチェック
        full_path = os.path.join(ROOT_DIR, file_path)
        absolute_root_dir = os.path.abspath(ROOT_DIR)
        absolute_full_path = os.path.abspath(full_path)

        if not absolute_full_path.startswith(absolute_root_dir):
            flash("許可されていないパスへのアクセスが試行されました。", "error")
            return redirect(url_for('browse', path=os.path.dirname(file_path)))
        
        if os.path.isdir(full_path):
            # フォルダが選択された場合は、そのフォルダの中を表示
            return redirect(url_for('browse', path=file_path))
        
        try:
            # 選択されたファイルの絶対パス (full_path) を使用
            command = f"echo 'ユーザーが選択したファイルパス: {full_path}'"
            
            command_list = shlex.split(command)

            # 別スレッドで実行
            Thread(target=run_subprocess, args=(command_list,)).start()
            
            flash(f"ファイルパスがサブプロセスに渡されました: {full_path}", "success")
            
        except subprocess.CalledProcessError as e:
            flash(f"コマンドの実行に失敗しました: {e}", "error")
            print(f"Error: {e.stderr}")
        except FileNotFoundError:
            flash("実行しようとしたコマンドが見つかりません。", "error")
            
    # ファイル実行後、現在のディレクトリに戻る
    current_dir_path = os.path.dirname(file_path) if file_path else ''
    return redirect(url_for('browse', path=current_dir_path))

# ----------------------------------------------------

@app.route('/gui', methods=['GET', 'POST'])
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
            arguments = request.form.get("arguments", "").strip() # 引数を取得
            if check1_outdoor == 'checked' and check2_outdoor == 'checked' and check3_outdoor == 'checked':
                script_path = os.path.join(BASE_PATH, "nav_single_map.sh")
                # 引数をsubprocess.runに渡せる形式にする
                command_list = [script_path]
                if arguments:
                    command_list.extend(arguments.split())
                Thread(target=run_subprocess, args=(command_list,)).start()
                current_mode = "running"
                return redirect('/program_executed')
            else:
                return render_template('outdoor_run_popup.html', error="すべてのチェック項目にチェックを入れてください。")
        
        elif command.startswith("execute_mapping_"):
            # 🚨 マッピングはチェックボックスがないため、バリデーションなしで実行 🚨
            # 実行するマッピングの種類をコマンドから抽出
            mapping_type = command.replace("execute_mapping_", "")
            
            # マッピングスクリプトのパスと引数
            script_path = os.path.join(BASE_PATH, "start_mapping.sh") # 🚨 必要に応じてスクリプト名を変更してください 🚨
            command_list = [script_path, mapping_type]
            
            Thread(target=run_subprocess, args=(command_list,)).start()
            current_mode = "mapping"
            return redirect('/mapping_executed')

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
            return redirect(url_for('browse'))
        else:
            print(command)
            return "Unknown command", 400

if __name__ == '__main__':
    server = pywsgi.WSGIServer(('0.0.0.0', 5050), app, handler_class=WebSocketHandler)
    print("WebSocket Proxy server started at ws://0.0.0.0:5050/ws")
    server.serve_forever()