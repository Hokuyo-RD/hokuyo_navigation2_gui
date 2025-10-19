#!/usr/bin/env python3

import os
import shutil
import zipfile 
import subprocess
from threading import Thread
import asyncio
import yaml # YAMLをパースするため
import pathlib # パス操作のため
import re # 正規表現を使用するため

from flask import Flask, request, render_template, redirect, jsonify, url_for, flash, send_from_directory
from flask_sockets import Sockets
import websockets
from gevent import pywsgi
from geventwebsocket.handler import WebSocketHandler
from geventwebsocket.websocket import WebSocket

try:
    from rosbag2_filter_core import get_topic_list, filter_rosbag
except ImportError as e:
    print(f"Error: Core logic file (rosbag2_filter_core.py) or ROS 2 libraries not found/sourced: {e}")


# ==============================================================================
# 2. 設定と定数
# ==============================================================================

# --- パス設定 ---
# 環境変数 DOCKER_CONTAINER の有無でパスを分岐
if 'DOCKER_ENV' in os.environ:
    BASE_PATH = "/home/colcon_ws/src/hokuyo_navigation2/scripts/"
    HOKUYO_NAV2_PKG_PATH = "/home/colcon_ws/src/hokuyo_navigation2" 
else:
    BASE_PATH = "/home/hokuyo/colcon_ws/src/hokuyo_navigation2/scripts/"
    HOKUYO_NAV2_PKG_PATH = "/home/hokuyo/colcon_ws/src/hokuyo_navigation2"

ROSBAG_ROOT_DIR = os.path.join(HOKUYO_NAV2_PKG_PATH, 'rosbag')
DOWNLOAD_FOLDER = ROSBAG_ROOT_DIR 
ALLOWED_EXTENSIONS = {'bag', 'db3', 'mcap'}

MAP_DIR = os.path.join(HOKUYO_NAV2_PKG_PATH, 'map')
WP_DIR = os.path.join(HOKUYO_NAV2_PKG_PATH, 'waypoints')

ROSBRIDGE_URI = "ws://localhost:9090"
SERVER_HOST = '0.0.0.0'
SERVER_PORT = 5050

app = Flask(__name__)
sockets = Sockets(app)
app.secret_key = 'your_secret_key_here' 

current_mode = "stopped"

os.makedirs(ROSBAG_ROOT_DIR, exist_ok=True)
os.makedirs(DOWNLOAD_FOLDER, exist_ok=True)
os.makedirs(MAP_DIR, exist_ok=True)

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

async def forward(ws, target):
    """WebSocket間でメッセージを転送する"""
    try:
        while True:
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

@app.route('/browse_rosbag', defaults={'path': ''}) 
@app.route('/browse_rosbag/<path:path>')
def browse_rosbag(path):
    """ROS Bag フィルタ/同期/P2O/LIO-RAW用のファイルブラウザ"""
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
        
        sync_mode_browse = request.args.get('mode') == 'sync'
        p2o_mode_browse = request.args.get('mode') == 'p2o'
        lio_raw_mode_browse = request.args.get('mode') == 'lio_raw'
        
        return render_template('rosbag_browse.html', 
                               files=files, 
                               dirs=dirs, 
                               current_path=path, 
                               current_dir_name=os.path.basename(full_path) if path else ROSBAG_ROOT_DIR, 
                               root_dir=ROSBAG_ROOT_DIR,
                               parent_path=parent_path,
                               sync_mode=sync_mode_browse,
                               p2o_mode=p2o_mode_browse,
                               lio_raw_mode=lio_raw_mode_browse) 

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
    is_p2o_mode = request.form.get('mode') == 'p2o' 
    is_lio_raw_mode = request.form.get('mode') == 'lio_raw' 

    is_rosbag = os.path.isdir(full_path) or full_path.lower().endswith(tuple(f'.{ext}' for ext in ALLOWED_EXTENSIONS))
    
    if is_rosbag:
        try:
            topics_info = get_topic_list(full_path)
            topic_list = sorted(topics_info.keys())
            bag_duration_sec = 0
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

            if bag_duration_sec == 0 and os.path.isdir(full_path):
                metadata_path = pathlib.Path(full_path) / 'metadata.yaml'
                if metadata_path.exists():
                    with open(metadata_path, 'r') as f:
                        metadata = yaml.safe_load(f)
                        if 'duration' in metadata:
                            bag_duration_sec = metadata['duration'] / 1_000_000_000 
                        elif 'rosbag2_bagfile_information' in metadata and 'duration' in metadata['rosbag2_bagfile_information']:
                            bag_duration_sec = metadata['rosbag2_bagfile_information']['duration'] / 1_000_000_000
            
            if bag_duration_sec > 0:
                estimated_duration = round(bag_duration_sec)
            else:
                estimated_duration = 120
            
            flash(f'ROS Bag "{file_path}" を読み込みました。', 'success')
            
            return render_template('rosbag_select_topics.html', 
                                   topic_list=topic_list, 
                                   input_bag_path=full_path,
                                   sync_mode=is_sync_mode,
                                   p2o_mode=is_p2o_mode, 
                                   lio_raw_mode=is_lio_raw_mode,
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

    # 入力チェック 
    if not input_bag_path or not os.path.exists(input_bag_path):
        return jsonify({'status': 'error', 'message': '入力ファイルが見つかりません。パスを確認してください。'}), 400
    if not output_filename_base:
        return jsonify({'status': 'error', 'message': '出力ファイル名を入力してください。'}), 400
    if not is_sync_mode and not selected_topics:
        return jsonify({'status': 'error', 'message': 'トピックフィルタリングにはトピックを一つ以上選択してください。'}), 400

    base_name = os.path.basename(input_bag_path)
    if os.path.isfile(input_bag_path):
        base_name = os.path.splitext(base_name)[0]
    else:
        base_name = os.path.basename(input_bag_path.rstrip('/'))
        
    output_bag_dir = os.path.join(DOWNLOAD_FOLDER, output_filename_base)
    
    try:
        if is_sync_mode:
            script_path = os.path.join(BASE_PATH, "start_mapping.sh")
            command_list = [
                script_path, 
                "sync", 
                base_name,          # 選択したROS Bag名 (拡張子なし)
                output_filename_base # 新しいROS Bag名 (出力ディレクトリ名)
            ]
            
            Thread(target=run_subprocess, args=(command_list,)).start()
            result_message = f"トピック同期スクリプトがバックグラウンドで開始されました。出力ファイル名: {output_filename_base}。完了までお待ちください。"
            
        else:
            result_message = filter_rosbag(input_bag_path, output_bag_dir, selected_topics)

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

@app.route('/p2o_mapping', methods=['POST'])
def p2o_mapping():
    """P2O SLAM 処理を開始し、結果のダウンロードパスを返す (API)"""
    return _start_mapping_process('p2o', request)

@app.route('/lio_raw_mapping', methods=['POST'])
def lio_raw_mapping():
    """LIO-RAW SLAM 処理を開始し、結果のダウンロードパスを返す (API)"""
    return _start_mapping_process('lio_raw', request)

def _start_mapping_process(mode, req):
    """
    指定されたモードでマッピング処理を開始する共通ヘルパー関数。
    mode: 'p2o' または 'lio_raw'
    """
    try:
        data = req.get_json()
    except Exception as e:
        return jsonify({'status': 'error', 'message': f'リクエストJSONのパースエラー: {e}'}), 400

    input_bag_path = data.get('input_bag_path')
    output_map_name = data.get('output_map_name')

    if not input_bag_path or not os.path.exists(input_bag_path):
        return jsonify({'status': 'error', 'message': '入力ファイルが見つかりません。パスを確認してください。'}), 400
    if not output_map_name:
        return jsonify({'status': 'error', 'message': '出力マップ名を入力してください。'}), 400

    base_name = os.path.basename(input_bag_path)
    if os.path.isfile(input_bag_path):
        base_name = os.path.splitext(base_name)[0]
    else:
        base_name = os.path.basename(input_bag_path.rstrip('/'))

    flag_file_name_full = f'{output_map_name}.{mode.upper()}_DONE'
    
    try:
        script_path = os.path.join(BASE_PATH, "start_mapping.sh")
        command_list = [
            script_path, 
            mode,            # 0. モード ('p2o' または 'lio_raw')
            base_name,       # 1. 選択したROS Bag名
            output_map_name, # 2. 新しいマップ名 (PCD名)
            MAP_DIR,         # 3. マップ保存ディレクトリパス
            WP_DIR,          # 4. waypoints保存ディレクトリパス
            flag_file_name_full # 5. 完了フラグファイル名
        ]
        
        Thread(target=run_subprocess, args=(command_list,)).start()
        result_message = f"{mode.upper()} マッピングスクリプトがバックグラウンドで開始されました。出力マップ名: {output_map_name}"
        
        return jsonify({
            'status': 'success',
            'message': result_message,
            'map_name': output_map_name 
        })
        
    except Exception as e:
        print(f"ERROR: {mode.upper()} Mapping failed with exception: {e}")
        return jsonify({'status': 'error', 'message': f'処理中にエラーが発生しました: {e}'}), 500

def _check_mapping_status(mode, req):
    """
    P2O または LIO-RAW SLAM 処理の完了ステータスをチェックする共通ヘルパー関数。
    mode: 'p2o' または 'lio_raw'
    """
    data = req.get_json()
    output_map_name = data.get('output_map_name')

    if not output_map_name:
        return jsonify({'status': 'error', 'message': '出力マップ名が指定されていません。'}), 400

    flag_file_name = f'{output_map_name}.{mode.upper()}_DONE'
    flag_file_path = os.path.join(MAP_DIR, flag_file_name) 
    
    if os.path.exists(flag_file_path):
        pcd_filename = f'{output_map_name}.pcd'
        pcd_file_path = os.path.join(MAP_DIR, pcd_filename)
        
        try:
            os.remove(flag_file_path)
            print(f"{mode.upper()} completion flag removed: {flag_file_path}")
        except Exception as e:
            print(f"Warning: Failed to remove flag file {flag_file_path}: {e}")
            
        if not os.path.exists(pcd_file_path):
            return jsonify({
                'status': 'error', 
                'message': f'PCDファイルが見つかりません。フラグは存在しましたが、"{pcd_filename}" が {MAP_DIR} に見つかりません。'
            })

        download_url = url_for('download_map', filename=pcd_filename)
        
        return jsonify({
            'status': 'finished', 
            'message': f'{mode.upper()} SLAM 処理が完了しました。PCDファイルをダウンロードできます。',
            'map_name': output_map_name,
            'download_url': download_url 
        })
    else:
        return jsonify({
            'status': 'in_progress', 
            'message': f'{mode.upper()} SLAM 処理を続行中です...'
        })

@app.route('/check_p2o_status', methods=['POST'])
def check_p2o_status():
    """P2O SLAM 処理の完了ステータスをチェックするAPI。"""
    return _check_mapping_status('p2o', request)

@app.route('/check_lio_raw_status', methods=['POST'])
def check_lio_raw_status():
    """LIO-RAW SLAM 処理の完了ステータスをチェックするAPI。"""
    return _check_mapping_status('lio_raw', request)

@app.route('/check_sync_status', methods=['POST'])
def check_sync_status():
    """
    トピック同期処理の完了ステータスをチェックするAPI。
    """
    data = request.get_json()
    output_filename = data.get('output_filename')

    if not output_filename:
        return jsonify({'status': 'error', 'message': '出力ファイル名が指定されていません。'}), 400

    flag_file_name = f'{output_filename}.SYNC_DONE'
    flag_file_path = os.path.join(DOWNLOAD_FOLDER, flag_file_name) 
    
    if os.path.exists(flag_file_path):
        try:
            os.remove(flag_file_path)
            print(f"Sync completion flag removed: {flag_file_path}")
        except Exception as e:
            print(f"Warning: Failed to remove flag file {flag_file_path}: {e}")
            
        download_url = url_for('download_file', filename=output_filename)
            
        return jsonify({
            'status': 'finished', 
            'message': 'トピック同期処理が完了しました。',
            'download_path': output_filename,
            'download_url': download_url 
        })
    else:
        return jsonify({
            'status': 'in_progress', 
            'message': '処理を続行中です...'
        })


@app.route('/download/<path:filename>')
def download_file(filename):
    """フィルタリングされたBagディレクトリをZIP圧縮してダウンロードさせる (Filter/Sync用)"""
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

@app.route('/download_map/<filename>')
def download_map(filename):
    """
    PCDファイルをダウンロードさせる。(P2O/LIO-RAW完了後にフロントエンドから呼ばれる)
    """
    try:
        directory = MAP_DIR
        map_filename = filename 
        file_path = os.path.join(directory, map_filename)

        if not os.path.exists(file_path):
            flash(f'エラー: 指定されたマップファイル "{map_filename}" が {directory} に見つかりません。', 'error')
            return redirect(url_for('main_gui')) 

        return send_from_directory(
            directory, 
            map_filename, 
            as_attachment=True, 
            mimetype='application/octet-stream' # PCDファイルはバイナリデータ
        )

    except Exception as e:
        flash(f'マップファイルのダウンロード中にエラーが発生しました: {e}', 'error')
        return redirect(url_for('main_gui'))

@app.route('/view_map/<map_name>')
def view_map(map_name):
    """
    PCDビューアのHTMLページをレンダリングする。
    """
    if not map_name:
        flash("エラー: 表示するマップ名が指定されていません。", "error")
        return redirect(url_for('main_gui'))

    pcd_filename = f'{map_name}.pcd'
    pcd_file_path = os.path.join(MAP_DIR, pcd_filename)
    
    if not os.path.exists(pcd_file_path):
        flash(f'エラー: マップファイル "{pcd_filename}" がサーバーに見つかりません。', 'error')
        return redirect(url_for('main_gui'))
        
    return render_template('pcd_viewer.html', map_name=map_name)


@app.route('/map/<filename>')
def serve_map_file(filename):
    """
    pcd_viewer.htmlからリクエストされたPCDファイルをブラウザに提供する。
    """
    try:
        return send_from_directory(
            MAP_DIR, 
            filename, 
            as_attachment=False, # ダウンロードではなく、インライン表示（ビューアで利用）
            mimetype='application/octet-stream'
        )
    except Exception as e:
        print(f"ERROR: PCDファイルの提供に失敗しました: {e}")
        return jsonify({'error': 'ファイルが見つからないか、アクセスできません'}), 404
        
@app.route('/browse_pcd')
def browse_pcd():
    """PCD2PGM変換用のPCDファイルブラウザ。MAP_DIRから.pcdファイルを取得。"""
    try:
        if not _is_safe_path(MAP_DIR, HOKUYO_NAV2_PKG_PATH):
             flash("セキュリティ上の理由により、このディレクトリにはアクセスできません。", "error")
             return redirect(url_for('main_gui'))

        items = os.listdir(MAP_DIR)
        # .pcd 拡張子を持つファイルのみを抽出
        pcd_files = sorted([item for item in items if item.lower().endswith('.pcd')])
        
        return render_template('pcd_browse.html', 
                               pcd_files=pcd_files,
                               map_dir=MAP_DIR) 

    except (FileNotFoundError, PermissionError) as e:
        flash(f"マップディレクトリの操作中にエラーが発生しました: {e}", "error")
        return redirect(url_for('main_gui'))

@app.route('/pcd2pgm_select', methods=['POST'])
def pcd2pgm_select():
    """選択されたPCDファイル名を受け取り、変換設定画面へ遷移する"""
    pcd_filename = request.form.get('pcd_filename') 
    
    if not pcd_filename:
        flash("PCDファイルが選択されていません。", "error")
        return redirect(url_for('browse_pcd'))
    
    full_path = os.path.join(MAP_DIR, pcd_filename)
    if not _is_safe_path(full_path, MAP_DIR):
        flash("許可されていないパスへのアクセスが試行されました。", "error")
        return redirect(url_for('browse_pcd'))
    if not os.path.exists(full_path):
        flash(f"PCDファイルが見つかりません: {pcd_filename}", "error")
        return redirect(url_for('browse_pcd'))

    base_map_name = os.path.splitext(pcd_filename)[0]

    return render_template('pcd_pgm_convert.html', 
                           input_pcd_filename=pcd_filename,
                           default_output_name=f'{base_map_name}')

@app.route('/pcd2pgm_convert', methods=['POST'])
def pcd2pgm_convert():
    """PCD to PGM 変換処理を開始し、完了までポーリングするAPI"""
    try:
        data = request.get_json()
    except Exception as e:
        return jsonify({'status': 'error', 'message': f'リクエストJSONのパースエラー: {e}'}), 400

    input_pcd_filename = data.get('input_pcd_filename')
    output_map_name = data.get('output_map_name') # PGMファイルのベース名

    if not input_pcd_filename:
        return jsonify({'status': 'error', 'message': '入力PCDファイル名が指定されていません。'}), 400
    if not os.path.exists(os.path.join(MAP_DIR, input_pcd_filename)):
        return jsonify({'status': 'error', 'message': f'入力PCDファイルが見つかりません: {input_pcd_filename}'}), 400
    if not output_map_name:
        return jsonify({'status': 'error', 'message': '出力マップ名を入力してください。'}), 400
    
    FLAG_MODE = 'PCD2PGM'
    flag_file_name_full = f'{output_map_name}.{FLAG_MODE}_DONE'
    
    try:
        script_path = os.path.join(BASE_PATH, "start_mapping.sh")
        command_list = [
            script_path, 
            "pcd2pgm",                   # 0. モード
            input_pcd_filename,          # 1. 入力PCDファイル名 (例: my_map.pcd)
            output_map_name,             # 2. 出力PGMベース名 (例: my_map_pgm)
            MAP_DIR,                     # 3. マップ保存ディレクトリパス
            flag_file_name_full          # 4. 完了フラグファイル名
        ]
        
        # 別スレッドで実行
        Thread(target=run_subprocess, args=(command_list,)).start()
        result_message = f"PCD to PGM 変換スクリプトがバックグラウンドで開始されました。出力マップ名: {output_map_name}"
        
        return jsonify({
            'status': 'success',
            'message': result_message,
            'map_name': output_map_name 
        })
        
    except Exception as e:
        print(f"ERROR: PCD2PGM Mapping failed with exception: {e}")
        return jsonify({'status': 'error', 'message': f'処理中にエラーが発生しました: {e}'}), 500

@app.route('/check_pcd2pgm_status', methods=['POST'])
def check_pcd2pgm_status():
    """PCD to PGM 変換処理の完了ステータスをチェックするAPI。"""
    data = request.get_json()
    output_map_name = data.get('output_map_name')

    if not output_map_name:
        return jsonify({'status': 'error', 'message': '出力マップ名が指定されていません。'}), 400

    flag_file_name = f'{output_map_name}.PCD2PGM_DONE'
    flag_file_path = os.path.join(MAP_DIR, flag_file_name) 
    
    if os.path.exists(flag_file_path):
        pgm_filename = f'{output_map_name}.pgm'
        yaml_filename = f'{output_map_name}.yaml'
        pgm_file_path = os.path.join(MAP_DIR, pgm_filename)
        yaml_file_path = os.path.join(MAP_DIR, yaml_filename)
        
        try:
            os.remove(flag_file_path)
            print(f"PCD2PGM completion flag removed: {flag_file_path}")
        except Exception as e:
            print(f"Warning: Failed to remove flag file {flag_file_path}: {e}")
            
        if not os.path.exists(pgm_file_path) or not os.path.exists(yaml_file_path):
            return jsonify({
                'status': 'error', 
                'message': f'PGMまたはYAMLファイルが見つかりません。フラグは存在しましたが、"{pgm_filename}"または"{yaml_filename}"が{MAP_DIR}に見つかりません。'
            })
            
        return jsonify({
            'status': 'finished', 
            'message': 'PCD to PGM 変換処理が完了しました。マップファイルをダウンロードできます。',
            'map_name': output_map_name,
            'download_url': url_for('download_pgm_map', basename=output_map_name) 
        })
    else:
        return jsonify({
            'status': 'in_progress', 
            'message': 'PCD to PGM 変換処理を続行中です...'
        })

@app.route('/download_pgm_map/<basename>')
def download_pgm_map(basename):
    """PGMとYAMLファイルをZIP圧縮してダウンロードさせる"""
    temp_dir = os.path.join(DOWNLOAD_FOLDER, f'temp_pgm_{basename}')
    zip_base = os.path.join(DOWNLOAD_FOLDER, basename + '_pgm_map')
    zip_path = zip_base + '.zip'
    os.makedirs(temp_dir, exist_ok=True)
    
    pgm_filename = f'{basename}.pgm'
    yaml_filename = f'{basename}.yaml'
    
    try:
        shutil.copy2(os.path.join(MAP_DIR, pgm_filename), temp_dir)
        shutil.copy2(os.path.join(MAP_DIR, yaml_filename), temp_dir)        
        shutil.make_archive(zip_base, 'zip', temp_dir)
        
        zip_filename_only = os.path.basename(zip_path)

        return send_from_directory(
            DOWNLOAD_FOLDER, 
            zip_filename_only, 
            as_attachment=True
        )
    except FileNotFoundError:
        flash('ダウンロード用のマップファイルが見つかりません。', 'error')
        return redirect(url_for('main_gui'))
    except Exception as e:
        flash(f'ファイルのZIP化中にエラーが発生しました: {e}', 'error')
        return redirect(url_for('main_gui'))
    finally:
        # クリーンアップ
        if os.path.exists(temp_dir):
            shutil.rmtree(temp_dir)
        if os.path.exists(zip_path):
            os.remove(zip_path)

@app.route('/gui', methods=['GET', 'POST'])
def trigger_script():
    """GUIからのPOSTリクエストに基づき、対応するスクリプトを実行する。"""
    global current_mode
    if request.method == 'GET':
        return render_template('index.html') 
    
    command = request.form.get("command") or request.get_json().get("command")

    if command == "execute_indoor_run":
        map_name = request.form.get("map_name")
        if map_name:
            script_path = os.path.join(BASE_PATH, "expo_in")
            Thread(target=run_subprocess, args=([script_path],)).start()
            current_mode = "running"
            return redirect('/program_executed')
        else:
            return render_template('indoor_run_popup.html', error="すべてのチェック項目にチェックを入れてください。")
            
    elif command == "execute_outdoor_run":
        confirmation_field = request.form.get("confirm_check") 
        if confirmation_field or request.form: # フォームデータが存在するか、特定の確認フィールドがあるか
            script_path = os.path.join(BASE_PATH, "nav_single_map.sh")
            command_list = [script_path]

            Thread(target=run_subprocess, args=(command_list,)).start()
            current_mode = "running"
            return redirect('/program_executed')
        else:
            return render_template('outdoor_run_popup.html', error="開始を確認してください。")  
    elif command == "execute_mapping_sync":
        current_mode = "stopped"
        flash("トピック同期に使用するROS Bagを選択してください。", "info")
        return redirect(url_for('browse_rosbag', mode='sync')) 
    
    elif command == "execute_mapping_filter":
        current_mode = "stopped" 
        flash("ROS Bagフィルタリング機能に遷移します。フィルタ対象のROS Bagを選択してください。", "info")
        return redirect(url_for('browse_rosbag')) 
    
    elif command == "execute_mapping_p2o":
        current_mode = "stopped"
        flash("P2O マッピングに使用するROS Bagを選択してください。", "info")
        return redirect(url_for('browse_rosbag', mode='p2o'))

    elif command == "execute_mapping_lio_raw":
        current_mode = "stopped"
        flash("LIO-RAW マッピングに使用するROS Bagを選択してください。", "info")
        return redirect(url_for('browse_rosbag', mode='lio_raw'))

    elif command == "execute_mapping_pcd2pgm":
        current_mode = "stopped"
        flash("PGMマップに変換するPCDファイルを選択してください。", "info")
        return redirect(url_for('browse_pcd'))

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

if __name__ == '__main__':
    print(f"Flask Server starting at http://{SERVER_HOST}:{SERVER_PORT}")
    print(f"WebSocket Proxy server started at ws://{SERVER_HOST}:{SERVER_PORT}/ws")
    
    server = pywsgi.WSGIServer((SERVER_HOST, SERVER_PORT), app, handler_class=WebSocketHandler)
    server.serve_forever()