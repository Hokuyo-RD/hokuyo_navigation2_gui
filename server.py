#!/usr/bin/env python3

# ==============================================================================
# 1. インポート
# ==============================================================================

# 標準ライブラリ
import os
import shutil
import zipfile 
import subprocess
from threading import Thread
import asyncio

# 外部ライブラリ (Flask, WebSocket, Gevent)
from flask import Flask, request, render_template, redirect, jsonify, url_for, flash, send_from_directory
from flask_sockets import Sockets
import websockets
from gevent import pywsgi
from geventwebsocket.handler import WebSocketHandler
from geventwebsocket.websocket import WebSocket

# ローカルモジュール (ROS Bag フィルタのコアロジック)
try:
    from rosbag2_filter_core import get_topic_list, filter_rosbag
except ImportError as e:
    print(f"Error: Core logic file (rosbag2_filter_core.py) or ROS 2 libraries not found/sourced: {e}")


# ==============================================================================
# 2. 設定と定数
# ==============================================================================

# --- パス設定 ---
if 'DOCKER_CONTAINER' in os.environ:
    BASE_PATH = "/home/colcon_ws/src/hokuyo_navigation2/scripts/"
else:
    BASE_PATH = "/home/hokuyo/colcon_ws/src/hokuyo_navigation2/scripts/"

# ROS Bag フィルタのルートディレクトリと設定
ROSBAG_ROOT_DIR = '/home/hokuyo/colcon_ws/src/hokuyo_navigation2/rosbag'
DOWNLOAD_FOLDER = ROSBAG_ROOT_DIR # ダウンロードフォルダはROS Bagルートと同じ
ALLOWED_EXTENSIONS = {'bag', 'db3', 'mcap'}

# WebSocket 設定
ROSBRIDGE_URI = "ws://localhost:9090"
SERVER_HOST = '0.0.0.0'
SERVER_PORT = 5050


# ==============================================================================
# 3. Flaskアプリケーション初期化とグローバル変数
# ==============================================================================

app = Flask(__name__)
sockets = Sockets(app)
app.secret_key = 'your_secret_key_here' 

# グローバル変数
current_mode = "stopped"

# フォルダが存在しない場合は作成
os.makedirs(ROSBAG_ROOT_DIR, exist_ok=True)
os.makedirs(DOWNLOAD_FOLDER, exist_ok=True)


# ==============================================================================
# 4. ヘルパー関数 (ユーティリティ)
# ==============================================================================

def zip_directory(path, zip_filename):
    """指定されたパスのディレクトリをZIPファイルに圧縮する"""
    with zipfile.ZipFile(zip_filename, 'w', zipfile.ZIP_DEFLATED) as zipf:
        root_dir_name = os.path.basename(path)
        for root, _, files in os.walk(path):
            for file in files:
                file_path = os.path.join(root, file)
                archive_path = os.path.join(root_dir_name, os.path.relpath(file_path, path))
                zipf.write(file_path, archive_path)

def run_subprocess(command_list):
    """別スレッドでサブプロセスを実行するための関数"""
    # shlexは不要と判断し削除。リスト形式のコマンドを直接実行する。
    subprocess.run(command_list)


# ==============================================================================
# 5. WebSocket プロキシ処理
# ==============================================================================

async def forward(ws, target):
    """WebSocket間でメッセージを転送する"""
    try:
        while True:
            # Gevent WebSocketとasyncio WebSocketのrecv/receiveを区別
            message = await ws.recv() if isinstance(ws, websockets.legacy.client.WebSocketClientProtocol) else ws.receive()
            if message is None and isinstance(ws, WebSocket):
                break
            await target.send(message)
    except (websockets.exceptions.ConnectionClosedOK, websockets.exceptions.ConnectionClosedError, Exception):
        pass

async def proxy(websocket):
    """クライアントとROSBridge間のWebSocketプロキシを確立する"""
    ros_ws = None
    try:
        ros_ws = await websockets.connect(ROSBRIDGE_URI)
        await asyncio.gather(
            forward(websocket, ros_ws),
            forward(ros_ws, websocket),
        )
    except ConnectionRefusedError:
        print(f"Proxy: Error: Could not connect to ROSBridge at {ROSBRIDGE_URI}. Make sure it's running.")
    except Exception:
        pass
    finally:
        if ros_ws and not ros_ws.closed:
            await ros_ws.close()

@sockets.route('/ws')
def websocket_handler(ws):
    """WebSocket接続を処理し、asyncioイベントループでプロキシを実行する"""
    asyncio.run(proxy(ws))


# ==============================================================================
# 6. GUI/情報取得ルート
# ==============================================================================

@app.route('/')
@app.route('/gui')
def main_gui():
    """メインGUIページ (index.html)"""
    return render_template('index.html') 

@app.route('/get_mode')
def get_mode():
    """現在のシステムモードを返す (API)"""
    global current_mode
    return jsonify(mode=current_mode)

@app.route('/indoor_run')
def indoor_run():
    """屋内実行画面"""
    return render_template('indoor_run.html')

@app.route('/indoor_run_popup')
def indoor_run_popup():
    """屋内実行ポップアップ"""
    return render_template('indoor_run_popup.html')

@app.route('/outdoor_run_popup')
def outdoor_run_popup():
    """屋外実行ポップアップ"""
    return render_template('outdoor_run_popup.html')

@app.route('/stop')
def stop_run():
    """停止画面"""
    return render_template('stop.html')

@app.route('/mapping_executed')
def mapping_run():
    """マッピング実行後のメッセージ画面"""
    return render_template('demo_executed.html', message="マッピングが開始されました。安全に注意し、周囲を走行してください。")

@app.route('/mapping_popup')
def mapping_run_popup():
    """マッピング選択ポップアップ"""
    return render_template('mapping_popup.html')

@app.route('/demo_executed')
def demo_run():
    """デモ/手動操作実行後のメッセージ画面"""
    return render_template('demo_executed.html', message="手動操作モードに切り替わりました。Viewerでジョイスティックを使ってデモをしてください。")

@app.route('/program_executed')
def program_executed():
    """自律走行プログラム実行後のメッセージ画面"""
    return render_template('program_executed.html', message="自律走行が開始されました。周囲の安全に気をつけて下さい。")


# ==============================================================================
# 7. ROS Bag フィルタ機能ルート
# ==============================================================================

def _is_safe_path(full_path, root_dir):
    """ディレクトリトラバーサル攻撃を防ぐための安全なパスチェック"""
    absolute_root_dir = os.path.abspath(root_dir)
    absolute_full_path = os.path.abspath(full_path)
    return absolute_full_path.startswith(absolute_root_dir)

@app.route('/browse_rosbag', defaults={'path': ''}) 
@app.route('/browse_rosbag/<path:path>')
def browse_rosbag(path):
    """ROS Bag フィルタ用のファイルブラウザ"""
    full_path = os.path.join(ROSBAG_ROOT_DIR, path)
    
    if not _is_safe_path(full_path, ROSBAG_ROOT_DIR):
        flash("セキュリティ上の理由により、このディレクトリにはアクセスできません。", "error")
        return redirect(url_for('browse_rosbag'))
    
    if not os.path.isdir(full_path):
        flash(f"ディレクトリが見つかりません: {path}", "error")
        return redirect(url_for('browse_rosbag', path=os.path.dirname(path)))

    try:
        items = os.listdir(full_path)
        files = [item for item in items if os.path.isfile(os.path.join(full_path, item))]
        dirs = [item for item in items if os.path.isdir(os.path.join(full_path, item))]
        
        parent_path = os.path.dirname(path) if path else None

        return render_template('rosbag_browse.html', 
                               files=files, 
                               dirs=dirs, 
                               current_path=path, 
                               current_dir_name=os.path.basename(full_path) if path else ROSBAG_ROOT_DIR, 
                               root_dir=ROSBAG_ROOT_DIR,
                               parent_path=parent_path)

    except (FileNotFoundError, PermissionError) as e:
        flash(f"ディレクトリ操作中にエラーが発生しました: {e}", "error")
        return redirect(url_for('browse_rosbag'))

@app.route('/select_rosbag', methods=['POST'])
def select_rosbag():
    """ファイル/ディレクトリパスからトピックリストを取得し、選択画面へ遷移する"""
    file_path = request.form.get('file_path') 
    
    if not file_path:
        flash("ファイルまたはディレクトリが選択されていません。", "error")
        return redirect(url_for('browse_rosbag'))
    
    full_path = os.path.join(ROSBAG_ROOT_DIR, file_path)

    if not _is_safe_path(full_path, ROSBAG_ROOT_DIR):
        flash("許可されていないパスへのアクセスが試行されました。", "error")
        return redirect(url_for('browse_rosbag', path=os.path.dirname(file_path)))
    
    is_rosbag = os.path.isdir(full_path) or full_path.lower().endswith(tuple(f'.{ext}' for ext in ALLOWED_EXTENSIONS))
    
    if is_rosbag:
        try:
            topics_info = get_topic_list(full_path)
            topic_list = sorted(topics_info.keys())
            
            flash(f'ROS Bag "{file_path}" を読み込みました。トピックを選択してください。', 'success')
            
            return render_template('rosbag_select_topics.html', 
                                   topic_list=topic_list, 
                                   input_bag_path=full_path)
            
        except NameError:
            flash("ROS Bagフィルタのコア機能がインポートされていません。ROS環境を確認してください。", 'error')
            return redirect(url_for('browse_rosbag', path=os.path.dirname(file_path)))
        except Exception as e:
            flash(f'ROS Bagの読み込み中にエラーが発生しました: {e}', 'error')
            return redirect(url_for('browse_rosbag', path=os.path.dirname(file_path)))
    else:
        if os.path.isdir(full_path):
            return redirect(url_for('browse_rosbag', path=file_path))
        
        flash("選択されたファイル形式はROS Bagとしてサポートされていません。", "error")
        return redirect(url_for('browse_rosbag', path=os.path.dirname(file_path)))

@app.route('/convert', methods=['POST'])
def convert():
    """トピックフィルタリングを実行し、結果のダウンロードパスを返す (API)"""
    input_bag_path = request.form.get('input_bag_path')
    selected_topics = request.form.getlist('topics')
    output_filename_base = request.form.get('output_filename')

    # 入力チェック
    if not input_bag_path or not os.path.exists(input_bag_path):
        return jsonify({'status': 'error', 'message': '入力ファイルが見つかりません。パスを確認してください。'}), 400
    if not selected_topics:
        return jsonify({'status': 'error', 'message': 'トピックを一つ以上選択してください。'}), 400
        
    # 出力ファイル名の決定ロジック
    if not output_filename_base:
        base_name = os.path.basename(os.path.dirname(input_bag_path)) if os.path.isfile(input_bag_path) else os.path.basename(input_bag_path)
        if not base_name or base_name == '.':
             base_name = 'untitled_bag'
        output_filename_base = f'{base_name}_filtered'
    
    output_bag_dir = os.path.join(DOWNLOAD_FOLDER, output_filename_base)
    
    try:
        result_message = filter_rosbag(input_bag_path, output_bag_dir, selected_topics)
        
        print(f"DEBUG: Conversion finished. Result: {result_message}") 
        
        return jsonify({
            'status': 'success',
            'message': result_message,
            'download_path': output_filename_base 
        })
        
    except NameError:
        return jsonify({'status': 'error', 'message': 'ROS Bagフィルタのコア機能がインポートされていません。ROS環境を確認してください。'}), 500
    except Exception as e:
        print(f"ERROR: Conversion failed with exception: {e}")
        return jsonify({'status': 'error', 'message': f'変換中にエラーが発生しました: {e}'}), 500

@app.route('/download/<path:filename>')
def download_file(filename):
    """フィルタリングされたBagディレクトリをZIP圧縮してダウンロードさせる"""
    bag_dir_path = os.path.join(DOWNLOAD_FOLDER, filename)
    zip_filename = f'{filename}.zip'
    zip_path = os.path.join(DOWNLOAD_FOLDER, zip_filename)
    
    if not os.path.isdir(bag_dir_path):
        flash('ダウンロード用のファイルが見つかりません。', 'error')
        return redirect(url_for('browse_rosbag'))

    try:
        zip_directory(bag_dir_path, zip_path)
        
        return send_from_directory(
            DOWNLOAD_FOLDER, 
            zip_filename, 
            as_attachment=True
        )
    except Exception as e:
        flash(f'ファイルのZIP化中にエラーが発生しました: {e}', 'error')
        return redirect(url_for('browse_rosbag'))
    finally:
        if os.path.exists(zip_path):
            os.remove(zip_path)


# ==============================================================================
# 8. コマンド実行ルート (/gui POST)
# ==============================================================================

@app.route('/gui', methods=['GET', 'POST'])
def trigger_script():
    """
    GUIからのPOSTリクエストに基づき、対応するスクリプトを実行する。
    GETリクエストはメインGUIページを返す。
    """
    global current_mode
    if request.method == 'GET':
        return render_template('index.html') 
    
    # POST処理
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
        arguments = request.form.get("arguments", "").strip() 
        if check1_outdoor == 'checked' and check2_outdoor == 'checked' and check3_outdoor == 'checked':
            script_path = os.path.join(BASE_PATH, "nav_single_map.sh")
            command_list = [script_path]
            if arguments:
                command_list.extend(arguments.split())
            Thread(target=run_subprocess, args=(command_list,)).start()
            current_mode = "running"
            return redirect('/program_executed')
        else:
            return render_template('outdoor_run_popup.html', error="すべてのチェック項目にチェックを入れてください。")
    
    elif command == "execute_mapping_filter":
        current_mode = "stopped" 
        flash("ROS Bagフィルタリング機能に遷移します。フィルタ対象のROS Bagを選択してください。", "info")
        return redirect(url_for('browse_rosbag')) 
    
    elif command.startswith("execute_mapping_"):
        mapping_type = command.replace("execute_mapping_", "")
        
        script_path = os.path.join(BASE_PATH, "start_maping.sh")
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
        return redirect('/mapping_popup')
        
    else:
        print(f"Unknown command received: {command}")
        return "Unknown command", 400


# ==============================================================================
# 9. メインエントリーポイント
# ==============================================================================

if __name__ == '__main__':
    print(f"Flask Server starting at http://{SERVER_HOST}:{SERVER_PORT}")
    print(f"WebSocket Proxy server started at ws://{SERVER_HOST}:{SERVER_PORT}/ws")
    
    # Gevent WSGIサーバーでWebSocketとFlaskを同時にホスト
    server = pywsgi.WSGIServer((SERVER_HOST, SERVER_PORT), app, handler_class=WebSocketHandler)
    server.serve_forever()