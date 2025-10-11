#!/usr/bin/env python3

from flask import Flask, request, render_template, redirect, jsonify, url_for, flash
from flask_sockets import Sockets
import asyncio
import websockets
import os
import shutil
import zipfile # ROS Bag フィルタ機能用
import subprocess
from threading import Thread
from gevent import pywsgi
from geventwebsocket.handler import WebSocketHandler
from geventwebsocket.websocket import WebSocket
import shlex 

# --- ROS Bag フィルタのコアロジックをインポート ---
try:
    # 実際には rosbag2_filter_core.py ファイルが必要です
    # 存在しない場合でも、サーバーは起動し、エラーは無視されます。
    from rosbag2_filter_core import get_topic_list, filter_rosbag
except ImportError as e:
    print(f"Error: Core logic file (rosbag2_filter_core.py) or ROS 2 libraries not found/sourced: {e}")

# --- パス設定 ---
# 既存の自律走行スクリプトのベースパス
if 'DOCKER_CONTAINER' in os.environ:
    BASE_PATH = "/home/colcon_ws/src/hokuyo_navigation2/scripts/"
    MAPPING_ROOT_DIR = "/home/colcon_ws/src/hokuyo_navigation2"
else:
    BASE_PATH = "/home/hokuyo/colcon_ws/src/hokuyo_navigation2/scripts/"
    MAPPING_ROOT_DIR = "/home/hokuyo/colcon_ws/src/hokuyo_navigation2"

# ROS Bag フィルタ機能のルートディレクトリ
ROSBAG_ROOT_DIR = '/home/hokuyo/colcon_ws/src/hokuyo_navigation2/rosbag'

# ROS Bag フィルタのダウンロードフォルダ
DOWNLOAD_FOLDER = ROSBAG_ROOT_DIR
ALLOWED_EXTENSIONS = {'bag', 'db3', 'mcap'}

# フォルダが存在しない場合は作成
os.makedirs(ROSBAG_ROOT_DIR, exist_ok=True)
os.makedirs(DOWNLOAD_FOLDER, exist_ok=True)
# -----------------

app = Flask(__name__)
sockets = Sockets(app)
app.secret_key = 'your_secret_key_here' # 任意の秘密鍵を設定してください

# グローバル変数で現在のモードを管理
current_mode = "stopped"
ROSBRIDGE_URI = "ws://localhost:9090"

# WebSocket関連の関数 (省略 - 変更なし)
async def forward(ws, target):
    try:
        while True:
            try:
                message = await ws.recv() if isinstance(ws, websockets.legacy.client.WebSocketClientProtocol) else ws.receive()
                if message is None and isinstance(ws, WebSocket):
                    break
                await target.send(message)
            except (websockets.exceptions.ConnectionClosedOK, websockets.exceptions.ConnectionClosedError):
                break
            except Exception:
                break
    finally:
        pass

async def proxy(websocket):
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
    asyncio.run(proxy(ws))

# ユーティリティ: ディレクトリをZIP圧縮する (ROS Bag フィルタ機能用)
def zip_directory(path, zip_filename):
    """指定されたパスのディレクトリをZIPファイルに圧縮する"""
    with zipfile.ZipFile(zip_filename, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for root, dirs, files in os.walk(path):
            for file in files:
                file_path = os.path.join(root, file)
                zipf.write(file_path, os.path.relpath(file_path, os.path.dirname(path)))

def run_subprocess(command_list):
    """別スレッドでサブプロセスを実行するための関数"""
    subprocess.run(command_list)

# ----------------------------------------------------
# 既存の自律走行GUI関連のルート
# ----------------------------------------------------
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
    return render_template('demo_executed.html', message="マッピングが開始されました。安全に注意し、周囲を走行してください。")

@app.route('/mapping_popup')
def mapping_run_popup():
    return render_template('mapping_popup.html')

@app.route('/demo_executed')
def demo_run():
    return render_template('demo_executed.html', message="手動操作モードに切り替わりました。Viewerでジョイスティックを使ってデモをしてください。")

@app.route('/program_executed')
def program_executed():
    return render_template('program_executed.html', message="自律走行が開始されました。周囲の安全に気をつけて下さい。")

# ----------------------------------------------------
# 統合されたROS Bag フィルタ機能のルート (ファイルブラウザとして利用)
# ----------------------------------------------------

@app.route('/', defaults={'path': ''})
@app.route('/browse_rosbag', defaults={'path': ''}) # ★ROS Bag機能のエントリポイント
@app.route('/browse_rosbag/<path:path>')
def browse_rosbag(path):
    """ROS Bag フィルタ用のファイルブラウザ"""
    global ROSBAG_ROOT_DIR
    
    full_path = os.path.join(ROSBAG_ROOT_DIR, path)
    
    # セキュリティチェック
    absolute_root_dir = os.path.abspath(ROSBAG_ROOT_DIR)
    absolute_full_path = os.path.abspath(full_path)
    
    if not absolute_full_path.startswith(absolute_root_dir):
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

        # テンプレート名を 'browse.html' から 'rosbag_browse.html' に変更
        return render_template('rosbag_browse.html', 
                               files=files, 
                               dirs=dirs, 
                               current_path=path, 
                               current_dir_name=os.path.basename(full_path) if path else absolute_root_dir, 
                               root_dir=ROSBAG_ROOT_DIR,
                               parent_path=parent_path)

    except FileNotFoundError:
        flash("指定されたディレクトリが見つかりません。", "error")
        return redirect(url_for('browse_rosbag'))
    except PermissionError:
        flash("ディレクトリへのアクセス権限がありません。", "error")
        return redirect(url_for('browse_rosbag'))

@app.route('/select_rosbag', methods=['POST'])
def select_rosbag():
    """クライアントから送られたファイル/ディレクトリパスからトピックリストを取得する"""
    global ROSBAG_ROOT_DIR
    
    file_path = request.form.get('file_path') 
    
    if not file_path:
        flash("ファイルまたはディレクトリが選択されていません。", "error")
        return redirect(url_for('browse_rosbag'))
    
    full_path = os.path.join(ROSBAG_ROOT_DIR, file_path)
    absolute_root_dir = os.path.abspath(ROSBAG_ROOT_DIR)
    absolute_full_path = os.path.abspath(full_path)

    if not absolute_full_path.startswith(absolute_root_dir):
        flash("許可されていないパスへのアクセスが試行されました。", "error")
        return redirect(url_for('browse_rosbag', path=os.path.dirname(file_path)))
    
    is_rosbag = os.path.isdir(full_path) or full_path.lower().endswith(tuple(f'.{ext}' for ext in ALLOWED_EXTENSIONS))
    
    if is_rosbag:
        try:
            topics_info = get_topic_list(full_path)
            topic_list = sorted(topics_info.keys())
            
            flash(f'ROS Bag "{file_path}" を読み込みました。トピックを選択してください。', 'success')
            
            # テンプレート名を 'select_topics.html' から 'rosbag_select_topics.html' に変更
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
    # ... (ROS Bag フィルタ機能から変更なし)
    input_bag_path = request.form.get('input_bag_path')
    selected_topics = request.form.getlist('topics')
    output_filename_base = request.form.get('output_filename')

    if not input_bag_path or not os.path.exists(input_bag_path):
        return jsonify({'status': 'error', 'message': '入力ファイルが見つかりません。パスを確認してください。'}), 400
    if not selected_topics:
        return jsonify({'status': 'error', 'message': 'トピックを一つ以上選択してください。'}), 400

    if not output_filename_base:
        base_name = os.path.basename(input_bag_path).split('.')[0]
        output_filename_base = f'{base_name}_filtered'
    
    output_bag_dir = os.path.join(DOWNLOAD_FOLDER, output_filename_base)
    
    try:
        # filter_rosbag 関数が NameError を起こす可能性があるため try-except に追加
        result_message = filter_rosbag(input_bag_path, output_bag_dir, selected_topics)
        
        return jsonify({
            'status': 'success',
            'message': result_message,
            'download_path': output_filename_base 
        })
        
    except NameError:
        return jsonify({'status': 'error', 'message': 'ROS Bagフィルタのコア機能がインポートされていません。ROS環境を確認してください。'}), 500
    except Exception as e:
        return jsonify({'status': 'error', 'message': f'変換中にエラーが発生しました: {e}'}), 500

@app.route('/download/<path:filename>')
def download_file(filename):
    # ... (ROS Bag フィルタ機能から変更なし)
    bag_dir_path = os.path.join(DOWNLOAD_FOLDER, filename)
    zip_filename = f'{filename}.zip'
    zip_path = os.path.join(DOWNLOAD_FOLDER, zip_filename)
    
    if not os.path.isdir(bag_dir_path):
        flash('ダウンロード用のファイルが見つかりません。', 'error')
        return redirect(url_for('browse_rosbag')) # ★リダイレクト先を修正

    try:
        zip_directory(bag_dir_path, zip_path)
        
        return send_from_directory(
            DOWNLOAD_FOLDER, 
            zip_filename, 
            as_attachment=True
        )
    except Exception as e:
        flash(f'ファイルのZIP化中にエラーが発生しました: {e}', 'error')
        return redirect(url_for('browse_rosbag')) # ★リダイレクト先を修正
    finally:
        if os.path.exists(zip_path):
            os.remove(zip_path)

# ----------------------------------------------------
# 既存の自律走行GUIのロジック
# ----------------------------------------------------

@app.route('/gui', methods=['GET', 'POST'])
def trigger_script():
    global current_mode
    if request.method == 'GET':
        # ★元のメインページ（自律走行GUI）のHTMLテンプレートを指定
        return render_template('index.html') 
    elif request.method == 'POST':
        command = request.form.get("command") or request.get_json().get("command")

        # ... (他の自律走行コマンドのロジックは変更なし)
        # ... (execute_indoor_run, execute_outdoor_run)
        
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
                command_list = [script_path]
                if arguments:
                    command_list.extend(arguments.split())
                Thread(target=run_subprocess, args=(command_list,)).start()
                current_mode = "running"
                return redirect('/program_executed')
            else:
                return render_template('outdoor_run_popup.html', error="すべてのチェック項目にチェックを入れてください。")
        
        # 🌟 変更点: start_mapping_filter コマンドの処理 🌟
        elif command == "execute_mapping_filter":
            # マッピングフィルター機能のファイルブラウザにリダイレクト
            current_mode = "stopped" # モードをstoppedに戻すか、専用のモードを設定
            flash("ROS Bagフィルタリング機能に遷移します。フィルタ対象のROS Bagを選択してください。", "info")
            return redirect(url_for('browse_rosbag')) # ROS Bagブラウザのルートに遷移
        
        elif command.startswith("execute_mapping_"):
            # 既存のマッピング実行ロジック (start_mapping.sh)
            mapping_type = command.replace("execute_mapping_", "")
            
            # start_maping.sh に修正 (以前の指示に基づく)
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
            # 既存のファイルブラウザ機能（ファイル実行用）は削除されていないが、
            # ROS Bag機能が/browse_rosbagに分離されたため、
            # この 'map' コマンドが元々何をしていたかによって調整が必要
            # 今回は /browse_rosbag が ROS Bag フィルタのメイン画面となるため、
            # 'map' は ROS Bag フィルタのブラウザに遷移するものと仮定します。
            # もし元の 'map' が別のファイルブラウザ ('/browse') を指していた場合は、
            # そのルートを復活させ、ここでリダイレクトしてください。
            flash("マッピングに関連するファイル処理は、現在ROS Bagフィルタ機能に統合されています。", "info")
            return redirect(url_for('browse_rosbag'))
        else:
            print(command)
            return "Unknown command", 400

if __name__ == '__main__':
    # Flaskのメインアプリケーションを '/gui' から起動するように修正 (例)
    # デフォルトの '/' は ROS Bag ブラウザのエイリアスとして残しています。
    server = pywsgi.WSGIServer(('0.0.0.0', 5050), app, handler_class=WebSocketHandler)
    print("WebSocket Proxy server started at ws://0.0.0.0:5050/ws")
    server.serve_forever()