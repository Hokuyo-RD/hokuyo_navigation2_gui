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
import yaml # YAMLをパースするため
import pathlib # パス操作のため
import re # 正規表現を使用するため

# 外部ライブラリ (Flask, WebSocket, Gevent)
from flask import Flask, request, render_template, redirect, jsonify, url_for, flash, send_from_directory
from flask_sockets import Sockets
import websockets
from gevent import pywsgi
from geventwebsocket.handler import WebSocketHandler
from geventwebsocket.websocket import WebSocket

# ローカルモジュール (ROS Bag フィルタのコアロジック)
try:
    # rosbag2_filter_core.py が必要（get_topic_list, filter_rosbag を提供）
    from rosbag2_filter_core import get_topic_list, filter_rosbag
except ImportError as e:
    print(f"Error: Core logic file (rosbag2_filter_core.py) or ROS 2 libraries not found/sourced: {e}")


# ==============================================================================
# 2. 設定と定数
# ==============================================================================

# --- パス設定 ---
# 環境変数 DOCKER_CONTAINER の有無でパスを分岐
if 'DOCKER_CONTAINER' in os.environ:
    BASE_PATH = "/home/colcon_ws/src/hokuyo_navigation2/scripts/"
else:
    BASE_PATH = "/home/hokuyo/colcon_ws/src/hokuyo_navigation2/scripts/"

# ROS Bag フィルタのルートディレクトリと設定
ROSBAG_ROOT_DIR = '/home/hokuyo/colcon_ws/src/hokuyo_navigation2/rosbag'
DOWNLOAD_FOLDER = ROSBAG_ROOT_DIR 
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
    try:
        # check=Trueでエラー発生時に例外を発生させる (元のロジック維持)
        subprocess.run(command_list, check=True, capture_output=True, text=True)
        print(f"Subprocess finished successfully: {command_list}")
    except subprocess.CalledProcessError as e:
        print(f"Subprocess failed: {e}")
        print(f"Stdout: {e.stdout}")
        print(f"Stderr: {e.stderr}")
    except FileNotFoundError:
        print(f"Subprocess failed: Command not found or script path error: {command_list}")


def _is_safe_path(full_path, root_dir):
    """ディレクトリトラバーサル攻撃を防ぐための安全なパスチェック"""
    absolute_root_dir = os.path.abspath(root_dir)
    absolute_full_path = os.path.abspath(full_path)
    return absolute_full_path.startswith(absolute_root_dir)


# ==============================================================================
# 5. WebSocket プロキシ処理
# ==============================================================================

async def forward(ws, target):
    """WebSocket間でメッセージを転送する"""
    try:
        while True:
            # Gevent/Flask-Socketsのwsオブジェクトとwebsocketsライブラリのオブジェクトを区別
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
    """マッピング実行後のメッセージ画面"""
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


# ==============================================================================
# 7. ROS Bag フィルタ/同期/P2O機能ルート
# ==============================================================================

@app.route('/browse_rosbag', defaults={'path': ''}) 
@app.route('/browse_rosbag/<path:path>')
def browse_rosbag(path):
    """ROS Bag フィルタ/同期/P2O用のファイルブラウザ"""
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
        
        # URLパラメータからモードを取得
        sync_mode_browse = request.args.get('mode') == 'sync'
        p2o_mode_browse = request.args.get('mode') == 'p2o'
        
        return render_template('rosbag_browse.html', 
                               files=files, 
                               dirs=dirs, 
                               current_path=path, 
                               current_dir_name=os.path.basename(full_path) if path else ROSBAG_ROOT_DIR, 
                               root_dir=ROSBAG_ROOT_DIR,
                               parent_path=parent_path,
                               # browse_rosbag.htmlの遷移先制御用
                               sync_mode=sync_mode_browse,
                               p2o_mode=p2o_mode_browse) 

    except (FileNotFoundError, PermissionError) as e:
        flash(f"ディレクトリ操作中にエラーが発生しました: {e}", "error")
        return redirect(url_for('browse_rosbag'))

@app.route('/select_rosbag', methods=['POST'])
def select_rosbag():
    """ファイル/ディレクトリパスからトピックリストを取得し、選択画面へ遷移する"""
    file_path = request.form.get('file_path') 
    
    if not file_path:
        mode = request.form.get('mode') 
        flash("ファイルまたはディレクトリが選択されていません。", "error")
        return redirect(url_for('browse_rosbag', mode=mode))
    
    full_path = os.path.join(ROSBAG_ROOT_DIR, file_path)

    if not _is_safe_path(full_path, ROSBAG_ROOT_DIR):
        flash("許可されていないパスへのアクセスが試行されました。", "error")
        return redirect(url_for('browse_rosbag', path=os.path.dirname(file_path)))
    
    is_sync_mode = request.form.get('mode') == 'sync' 
    is_p2o_mode = request.form.get('mode') == 'p2o' # 🌟 P2Oモード 🌟

    is_rosbag = os.path.isdir(full_path) or full_path.lower().endswith(tuple(f'.{ext}' for ext in ALLOWED_EXTENSIONS))
    
    if is_rosbag:
        try:
            topics_info = get_topic_list(full_path)
            topic_list = sorted(topics_info.keys())
            
            # ROS Bagの再生時間を取得するロジック (ros2 bag info 優先) 
            bag_duration_sec = 0
            
            # 1. ros2 bag info コマンドで秒数を取得 (省略)
            # ...
            command_list = ["ros2", "bag", "info", full_path]
            try:
                result = subprocess.run(
                    command_list, 
                    capture_output=True, 
                    text=True, 
                    check=True, 
                    timeout=10
                )
                
                for line in result.stdout.splitlines():
                    if "Duration:" in line:
                        match_direct = re.search(r'Duration:\s+([\d.]+?)s', line)
                        if match_direct:
                            bag_duration_sec = float(match_direct.group(1))
                            break
                        match_bracket = re.search(r'\(([\d.]+?)s\)', line)
                        if match_bracket:
                            bag_duration_sec = float(match_bracket.group(1))
                            break
                        
            except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired) as e:
                print(f"Warning: ros2 bag info failed or not found: {e}. Falling back to metadata.yaml.")

            # 2. metadata.yaml にフォールバック (省略)
            if bag_duration_sec == 0 and os.path.isdir(full_path):
                metadata_path = pathlib.Path(full_path) / 'metadata.yaml'
                if metadata_path.exists():
                    with open(metadata_path, 'r') as f:
                        metadata = yaml.safe_load(f)
                        if 'duration' in metadata:
                            bag_duration_sec = metadata['duration'] / 1_000_000_000 
                        elif 'rosbag2_bagfile_information' in metadata and 'duration' in metadata['rosbag2_bagfile_information']:
                            bag_duration_sec = metadata['rosbag2_bagfile_information']['duration'] / 1_000_000_000
            
            # 処理時間の目安として、取得したduration（秒）をそのまま使用（最小値なし）
            if bag_duration_sec > 0:
                # P2O/Sync処理はBagの長さに依存するため、Bagの長さを目安とする
                estimated_duration = round(bag_duration_sec)
            else:
                estimated_duration = 120
            # ---------------------
            
            flash(f'ROS Bag "{file_path}" を読み込みました。', 'success')
            
            return render_template('rosbag_select_topics.html', 
                                   topic_list=topic_list, 
                                   input_bag_path=full_path,
                                   sync_mode=is_sync_mode,
                                   p2o_mode=is_p2o_mode, # 🌟 P2Oモード 🌟
                                   # テンプレートに時間を渡す
                                   bag_duration_sec=estimated_duration) 
            
        except NameError:
            flash("ROS Bagフィルタのコア機能がインポートされていません。ROS環境を確認してください。", 'error')
            return redirect(url_for('browse_rosbag', path=os.path.dirname(file_path)))
        except Exception as e:
            flash(f'ROS Bagの読み込み中にエラーが発生しました: {e}', 'error')
            return redirect(url_for('browse_rosbag', path=os.path.dirname(file_path)))
    else:
        # ディレクトリの場合はブラウズを続行
        if os.path.isdir(full_path):
            mode = request.form.get('mode')
            return redirect(url_for('browse_rosbag', path=file_path, mode=mode))
        
        flash("選択されたファイル形式はROS Bagとしてサポートされていません。", "error")
        return redirect(url_for('browse_rosbag', path=os.path.dirname(file_path), mode=request.form.get('mode')))

@app.route('/convert', methods=['POST'])
def convert():
    """トピックフィルタリングまたはトピック同期を実行し、結果のダウンロードパスを返す (API)"""
    try:
        data = request.get_json()
    except Exception as e:
        return jsonify({'status': 'error', 'message': f'リクエストJSONのパースエラー: {e}'}), 400

    input_bag_path = data.get('input_bag_path')
    selected_topics = data.get('topics', [])
    output_filename_base = data.get('output_filename')
    is_sync_mode = data.get('is_sync_mode', False)

    # 入力チェック (省略)
    if not input_bag_path or not os.path.exists(input_bag_path):
        return jsonify({'status': 'error', 'message': '入力ファイルが見つかりません。パスを確認してください。'}), 400
    if not output_filename_base:
        return jsonify({'status': 'error', 'message': '出力ファイル名を入力してください。'}), 400
    if not is_sync_mode and not selected_topics:
        return jsonify({'status': 'error', 'message': 'トピックフィルタリングにはトピックを一つ以上選択してください。'}), 400

    # ROS Bagのディレクトリ名/ファイル名（拡張子なし）を取得 
    base_name = os.path.basename(input_bag_path)
    if os.path.isfile(input_bag_path):
        base_name = os.path.splitext(base_name)[0]
    else:
        base_name = os.path.basename(input_bag_path.rstrip('/'))
        
    output_bag_dir = os.path.join(DOWNLOAD_FOLDER, output_filename_base)
    
    try:
        if is_sync_mode:
            # 🌟 トピック同期処理: start_mapping.sh sync input_bag_name output_bag_name で実行 🌟
            script_path = os.path.join(BASE_PATH, "start_mapping.sh")
            command_list = [
                script_path, 
                "sync", 
                base_name,          # 選択したROS Bag名 (拡張子なし)
                output_filename_base # 新しいROS Bag名 (出力ディレクトリ名)
            ]
            
            # 別スレッドで実行
            Thread(target=run_subprocess, args=(command_list,)).start()
            result_message = f"トピック同期スクリプトがバックグラウンドで開始されました。出力ファイル名: {output_filename_base}。完了までお待ちください。"
            
        else:
            # トピックフィルタリング処理: filter_rosbag を実行
            result_message = filter_rosbag(input_bag_path, output_bag_dir, selected_topics)

        
        # クライアント側でポーリング/完了待機が必要なため、ここでは成功応答を返す。
        return jsonify({
            'status': 'success',
            'message': result_message,
            'download_path': output_filename_base 
        })
        
    except NameError:
        return jsonify({'status': 'error', 'message': 'ROS Bagフィルタのコア機能がインポートされていません。ROS環境を確認してください。'}), 500
    except Exception as e:
        print(f"ERROR: Conversion/Sync failed with exception: {e}")
        return jsonify({'status': 'error', 'message': f'処理中にエラーが発生しました: {e}'}), 500

# ------------------------------------------------------
# 🌟 新規: P2O マッピング実行用 API 🌟
# ------------------------------------------------------
@app.route('/p2o_mapping', methods=['POST'])
def p2o_mapping():
    """P2O SLAM 処理を開始し、結果のダウンロードパスを返す (API)"""
    try:
        data = request.get_json()
    except Exception as e:
        return jsonify({'status': 'error', 'message': f'リクエストJSONのパースエラー: {e}'}), 400

    input_bag_path = data.get('input_bag_path')
    output_map_name = data.get('output_map_name') # output_map_name は output_filename と同じ値が来る

    if not input_bag_path or not os.path.exists(input_bag_path):
        return jsonify({'status': 'error', 'message': '入力ファイルが見つかりません。パスを確認してください。'}), 400
    if not output_map_name:
        return jsonify({'status': 'error', 'message': '出力マップ名を入力してください。'}), 400

    # ROS Bagのディレクトリ名/ファイル名（拡張子なし）を取得 
    base_name = os.path.basename(input_bag_path)
    if os.path.isfile(input_bag_path):
        base_name = os.path.splitext(base_name)[0]
    else:
        base_name = os.path.basename(input_bag_path.rstrip('/'))
    
    try:
        # 🌟 P2O マッピング処理: start_mapping.sh p2o input_bag_name output_map_name で実行 🌟
        script_path = os.path.join(BASE_PATH, "start_mapping.sh")
        command_list = [
            script_path, 
            "p2o", 
            base_name,       # 選択したROS Bag名 (拡張子なし)
            output_map_name  # 新しいマップ名 (PCD名)
        ]
        
        # 別スレッドで実行
        Thread(target=run_subprocess, args=(command_list,)).start()
        result_message = f"P2O マッピングスクリプトがバックグラウンドで開始されました。出力マップ名: {output_map_name} (フラグファイル: {output_map_name}.P2O_DONE)"
        
        # クライアント側でポーリング/完了待機が必要なため、ここでは成功応答を返す。
        return jsonify({
            'status': 'success',
            'message': result_message,
            'map_name': output_map_name 
        })
        
    except Exception as e:
        print(f"ERROR: P2O Mapping failed with exception: {e}")
        return jsonify({'status': 'error', 'message': f'処理中にエラーが発生しました: {e}'}), 500

# ------------------------------------------------------
# 🌟 新規: P2O マッピング完了チェック用 API 🌟
# ------------------------------------------------------
@app.route('/check_p2o_status', methods=['POST'])
def check_p2o_status():
    """P2O SLAM 処理の完了ステータスをチェックするAPI。"""
    data = request.get_json()
    output_map_name = data.get('output_map_name')

    if not output_map_name:
        return jsonify({'status': 'error', 'message': '出力マップ名が指定されていません。'}), 400

    # 完了フラグファイルのパス
    flag_file_name = f'{output_map_name}.P2O_DONE'
    # DOWNLOAD_FOLDER は ROSBAG_ROOT_DIR と同じ
    flag_file_path = os.path.join(DOWNLOAD_FOLDER, flag_file_name) 
    
    # 完了ファイルが存在するかチェック
    if os.path.exists(flag_file_path):
        # 完了ファイルを削除して、次の実行に備える（クリーンアップ）
        try:
            os.remove(flag_file_path)
            print(f"P2O completion flag removed: {flag_file_path}")
        except Exception as e:
            print(f"Warning: Failed to remove flag file {flag_file_path}: {e}")
            
        # 処理終了を返す
        return jsonify({
            'status': 'finished', 
            'message': 'P2O SLAM 処理が完了しました。',
            'map_name': output_map_name
        })
    else:
        # 処理続行中を返す
        return jsonify({
            'status': 'in_progress', 
            'message': 'P2O SLAM 処理を続行中です...'
        })
# ------------------------------------------------------

# 既存の /check_sync_status は変更なしで残します
@app.route('/check_sync_status', methods=['POST'])
def check_sync_status():
    """
    トピック同期処理の完了ステータスをチェックするAPI。
    rosbag_record_with_timeout.bash が作成したフラグファイルの有無で判定する。
    """
    data = request.get_json()
    output_filename = data.get('output_filename')

    if not output_filename:
        return jsonify({'status': 'error', 'message': '出力ファイル名が指定されていません。'}), 400

    # 完了フラグファイルのパス
    flag_file_name = f'{output_filename}.SYNC_DONE'
    flag_file_path = os.path.join(DOWNLOAD_FOLDER, flag_file_name)
    
    # 完了ファイルが存在するかチェック
    if os.path.exists(flag_file_path):
        try:
            os.remove(flag_file_path)
            print(f"Sync completion flag removed: {flag_file_path}")
        except Exception as e:
            print(f"Warning: Failed to remove flag file {flag_file_path}: {e}")
            
        return jsonify({
            'status': 'finished', 
            'message': 'トピック同期処理が完了しました。',
            'download_path': output_filename
        })
    else:
        return jsonify({
            'status': 'in_progress', 
            'message': '処理を続行中です...'
        })


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
        # ZIPファイルを削除 (ダウンロード後のクリーンアップ)
        if os.path.exists(zip_path):
            os.remove(zip_path)


# ==============================================================================
# 8. コマンド実行ルート (/gui POST)
# ==============================================================================

@app.route('/gui', methods=['GET', 'POST'])
def trigger_script():
    """GUIからのPOSTリクエストに基づき、対応するスクリプトを実行する。"""
    global current_mode
    if request.method == 'GET':
        return render_template('index.html') 
    
    command = request.form.get("command") or request.get_json().get("command")

    if command == "execute_indoor_run":
        # ... (コードは省略) ...
        # ... (成功時) ...
        script_path = os.path.join(BASE_PATH, "expo_in")
        Thread(target=run_subprocess, args=([script_path],)).start()
        current_mode = "running"
        return redirect('/program_executed')
        # ... (エラー時) ...
        return render_template('indoor_run_popup.html', error="すべてのチェック項目にチェックを入れてください。")
            
    elif command == "execute_outdoor_run":
        # ... (コードは省略) ...
        # ... (成功時) ...
        script_path = os.path.join(BASE_PATH, "nav_single_map.sh")
        command_list = [script_path]
        arguments = request.form.get("arguments", "").strip() 
        if arguments:
            command_list.extend(arguments.split())
        Thread(target=run_subprocess, args=(command_list,)).start()
        current_mode = "running"
        return redirect('/program_executed')
        # ... (エラー時) ...
        return render_template('outdoor_run_popup.html', error="すべてのチェック項目にチェックを入れてください。")
    
    # トピック同期機能への遷移 
    elif command == "execute_mapping_sync":
        current_mode = "stopped"
        flash("トピック同期に使用するROS Bagを選択してください。", "info")
        return redirect(url_for('browse_rosbag', mode='sync')) 
    
    # ROS Bag filter の処理への遷移
    elif command == "execute_mapping_filter":
        current_mode = "stopped" 
        flash("ROS Bagフィルタリング機能に遷移します。フィルタ対象のROS Bagを選択してください。", "info")
        return redirect(url_for('browse_rosbag')) 
    
    # 🌟 P2Oマッピングへの遷移 🌟
    elif command == "execute_mapping_p2o":
        current_mode = "stopped"
        flash("P2O マッピングに使用するROS Bagを選択してください。", "info")
        return redirect(url_for('browse_rosbag', mode='p2o'))
    
    # マッピング処理全般（lio_raw, pcd2pgmなど）
    elif command.startswith("execute_mapping_"):
        mapping_type = command.replace("execute_mapping_", "")
        
        script_path = os.path.join(BASE_PATH, "start_mapping.sh")
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