"""実行ログを読み取り、非エンジニアにも分かる言葉に翻訳するモジュール。

マッピングやPCD変換は別ターミナルの bash / Python スクリプトとして動くため、
失敗しても英語のスタックトレースや「exit 1」しか残らない。
ここでは実行ログの1行1行を既知のパターンと突き合わせ、
「何が起きたか」「どうすればよいか」を日本語で説明する。

パターンに当てはまらない行も、ERROR / Traceback などの手掛かりがあれば
汎用の指摘として拾い上げる。何も当てはまらない行は無視する
（生ログは画面にそのまま表示されるので、詳細はそちらで確認できる）。
"""

import re

# サブプロセスの終了コードをログに残すための目印。
# start_mapping.sh がこの形式で1行出力する。
EXIT_MARKER = '[HOKUYO_GUI] exit_code='
EXIT_PATTERN = re.compile(r'\[HOKUYO_GUI\] exit_code=(-?\d+)')

# (キー, 正規表現, 深刻度, 見出し, 原因の説明, 対処)
# 上から順に照合し、最初に一致したものを採用する。
# 具体的なパターンを先に、汎用的なパターンを後ろに置くこと。
LOG_RULES = [
    (
        'bag_not_found',
        re.compile(r'Path .* does not exist|rosbag.*not found|Failed to read rosbag2', re.IGNORECASE),
        'error',
        'ROS Bag ファイルが見つかりません',
        '処理しようとしたROS Bagが、想定した場所にありませんでした。',
        'ROS Bagの選択画面に戻り、ファイルを選び直してください。'
        '別の画面でファイル名を変更・削除していないか確認してください。',
    ),
    (
        'binary_not_found',
        re.compile(r"'run_p2o' 実行ファイルが見つかりません|run_p2o.*not found"),
        'error',
        'SLAMの実行プログラムが見つかりません',
        'ビルドされた実行ファイル (run_p2o) が見つからないため、地図の最適化ができません。',
        'ソフトウェアのビルドが完了していません。システム管理者にビルドを依頼してください。',
    ),
    (
        'script_not_found',
        re.compile(r'(hokuyo_slam|lio_raw|pcd2pgm)\.bash.*(No such file|見つかりません)'),
        'error',
        '実行スクリプトが見つかりません',
        '処理を行うスクリプトファイルがインストールされていません。',
        'ソフトウェアが正しく取得できていない可能性があります。システム管理者に連絡してください。',
    ),
    (
        'topics_not_in_bag',
        re.compile(r'None of the target topics found|The configured topics were not found'),
        'error',
        '設定したトピックがROS Bagに入っていません',
        'パラメータ設定ファイルに書かれたトピック名が、このROS Bagのトピック名と違います。',
        'ROS Bag選択画面の「ROS Bag の内容」で実際のトピック名を確認し、'
        'pointcloud_topic と lio_topic を書き換えてください。',
    ),
    (
        'odom_topic_wrong',
        re.compile(r'Odometry data not found'),
        'error',
        'オドメトリのトピック名が違います',
        '設定したオドメトリ（自己位置）のトピックにデータがありませんでした。',
        'パラメータ設定ファイルの lio_topic を、ROS Bagに記録されているトピック名に直してください。',
    ),
    (
        'pointcloud_topic_wrong',
        re.compile(r'PointCloud data not found'),
        'error',
        '点群のトピック名が違います',
        '設定した点群（LiDAR）のトピックにデータがありませんでした。',
        'パラメータ設定ファイルの pointcloud_topic を、ROS Bagに記録されているトピック名に直してください。',
    ),
    (
        'frame_mismatch',
        re.compile(r'because the frames did not match|座標系 \(orig_frame / target_frame\)'),
        'error',
        '座標系の設定がROS Bagと一致していません',
        'orig_frame / target_frame がROS Bagの中身と違うため、点群が1つも積み上がりませんでした。',
        '実行ログに出ている「Found in the bag」の座標系に合わせて、'
        'パラメータ設定ファイルの orig_frame と target_frame を直してください。',
    ),
    (
        'map_not_created',
        re.compile(r'No point clouds were saved|No map file was created|地図ファイルが作成されていません'),
        'error',
        '地図が作られませんでした',
        '処理は最後まで動きましたが、地図に入れる点群が1点もありませんでした。',
        'トピック名と座標系の設定を確認してください。'
        'それでも直らない場合は、ROS Bagに点群が記録されているか確認してください。',
    ),
    (
        'gnss_fix_rate_unavailable',
        re.compile(r'GNSSのfix率を計算できませんでした'),
        'error',
        'GNSSのデータが見つかりません',
        '設定したGNSSトピックが見つからないため、測位の品質を判定できませんでした。',
        'パラメータ設定ファイルの gnss_topic を、ROS Bagに記録されているトピック名に直してください。',
    ),
    (
        'pointcloud_dump_failed',
        re.compile(r'dump_lidar_pointcloud\.py failed'),
        'error',
        '点群データを取り出せませんでした',
        '設定した点群トピックが、このROS Bagに入っていない可能性が高いです。',
        'パラメータ設定ファイルの pointcloud_topic を、ROS Bagに記録されているトピック名に直してください。',
    ),
    (
        'p2o_empty',
        re.compile(r'output\.p2o is empty|output\.p2o が空'),
        'error',
        '地図のもとになるデータが作られませんでした',
        'オドメトリまたはIMUのトピック名が違うため、位置の情報が1件も取り出せませんでした。',
        'パラメータ設定ファイルの lio_topic と imu_topic を、'
        'ROS Bagに記録されているトピック名に直してください。',
    ),
    (
        'p2o_failed',
        re.compile(r'run_p2o optimization failed|concat\.txt is empty'),
        'error',
        '地図の最適化に失敗しました',
        '位置の推定結果がまとまらず、地図を組み立てられませんでした。',
        'GNSSの精度やデータの記録時間が不足している可能性があります。'
        'データ取得をやり直すか、gnss_cov_thre の値を見直してください。',
    ),
    (
        'gnss_fix_rate_low',
        re.compile(r'fix トピックの共分散のfix率が|Fix率が低いため'),
        'warning',
        'GNSSの測位精度が低い状態です',
        '衛星測位が安定していない区間が多いため、地図の精度が落ちる可能性があります。',
        '空が開けた場所で計測し直すか、パラメータ設定ファイルの gnss_cov_thre を大きくしてください。',
    ),
    (
        'gnss_no_fix',
        re.compile(r'rosbag play でfixメッセージがあるかの確認'),
        'error',
        'GNSSのデータが読み取れませんでした',
        '設定したGNSSトピックにメッセージが無いか、共分散の条件を満たしていません。',
        'パラメータ設定ファイルの gnss_topic と gnss_cov_thre を確認してください。',
    ),
    (
        'extractor_failed',
        re.compile(r'pcd_tf_extractor\.py がエラーコード'),
        'error',
        'LIO-RAW の地図作成に失敗しました',
        '点群の積み上げ処理が途中で終了しました。',
        'トピック名と座標系名 (orig_frame / target_frame) の設定を確認してください。',
    ),
    (
        'pcd_missing',
        re.compile(r'入力PCDファイルが見つかりません|入力PCDファイル.*not found'),
        'error',
        '変換元のPCDファイルがありません',
        '3D地図 (PCDファイル) が見つからないため、2D地図に変換できません。',
        '先に P2O または LIO-RAW で3D地図を作成してください。',
    ),
    (
        'waypoint_missing',
        re.compile(r'ウェイポイントファイルが見つかりません'),
        'warning',
        'ウェイポイントファイルが見つかりません',
        'ウェイポイントなしで処理を続行します。地図にはウェイポイントが反映されません。',
        'ウェイポイントを反映したい場合は、ファイル名を確認して選び直してください。',
    ),
    (
        'config_missing',
        re.compile(r'Config file not found'),
        'warning',
        'パラメータ設定ファイルが読み込まれていません',
        '設定ファイルが見つからないため、既定値で処理を行います。',
        '意図した設定で処理したい場合は、設定ファイルを選び直してください。',
    ),
    (
        'colcon_build_failed',
        re.compile(r'colcon build がエラーコード|colcon build.*failed', re.IGNORECASE),
        'error',
        'ソフトウェアの再ビルドに失敗しました',
        '作成した地図をシステムに反映する処理でエラーが出ました。',
        '地図ファイル自体は作られている場合があります。システム管理者に確認してください。',
    ),
    (
        'python_module_missing',
        re.compile(r'ModuleNotFoundError|ImportError:'),
        'error',
        '必要なソフトウェア部品が入っていません',
        '処理に必要なPythonライブラリが見つかりません。',
        'セットアップが完了していない可能性があります。システム管理者に連絡してください。',
    ),
    (
        'command_not_found',
        re.compile(r'command not found|コマンドが見つかりません'),
        'error',
        'コマンドが見つかりません',
        'ROS 2 の環境設定が読み込まれていないか、必要なツールが入っていません。',
        'システム管理者に環境設定の確認を依頼してください。',
    ),
    (
        'permission_denied',
        re.compile(r'Permission denied|許可がありません'),
        'error',
        'ファイルを読み書きする権限がありません',
        '出力先のフォルダに書き込めませんでした。',
        'システム管理者にフォルダの権限設定を確認してもらってください。',
    ),
    (
        'disk_full',
        re.compile(r'No space left on device|ディスク.*不足'),
        'error',
        'ディスクの空き容量が足りません',
        '地図ファイルを保存する空き容量がありません。',
        '不要なROS Bagや地図をファイル管理画面から削除してください。',
    ),
    (
        'out_of_memory',
        re.compile(r'MemoryError|Killed$|Cannot allocate memory'),
        'error',
        'メモリが足りません',
        '処理の途中でメモリを使い切り、強制終了されました。',
        'ROS Bagの記録時間を短くするか、pc_save_distance を大きくしてデータ量を減らしてください。',
    ),
    (
        'traceback',
        re.compile(r'Traceback \(most recent call last\)'),
        'error',
        'プログラムの内部エラーが発生しました',
        '処理中に想定外のエラーが起きました。',
        '下の実行ログをそのままシステム管理者に送ってください。',
    ),
    (
        'file_not_found',
        re.compile(r'No such file or directory'),
        'error',
        'ファイルまたはフォルダが見つかりません',
        '処理に必要なファイルが見つかりませんでした。',
        '下の実行ログで、どのファイルが不足しているか確認してください。',
    ),
    # --- 以下は汎用の受け皿。上のどれにも当てはまらなかった行を拾う。 ---
    (
        'generic_error',
        re.compile(r'^\s*(ERROR|Error:|エラー[:：])'),
        'error',
        '処理中にエラーが発生しました',
        '',
        '下の実行ログで、エラーの内容を確認してください。',
    ),
    (
        'generic_warning',
        re.compile(r'^\s*(WARNING|Warning:|警告[:：])'),
        'warning',
        '注意が必要なメッセージが出ています',
        '',
        '処理は続行されますが、結果を確認してください。',
    ),
]


def classify_line(line):
    """ログ1行を分類する。該当しなければ None。"""
    text = line.strip()
    if not text or text.startswith(EXIT_MARKER):
        return None
    for key, pattern, level, title, detail, hint in LOG_RULES:
        if pattern.search(text):
            return {
                'key': key,
                'level': level,
                'title': title,
                'detail': detail,
                'hint': hint,
                'raw': text[:500],
            }
    return None


def classify_lines(lines):
    """複数行をまとめて分類する。同じ種類の指摘は最初の1件だけ残す。"""
    events = []
    seen = set()
    for line in lines:
        event = classify_line(line)
        if event and event['key'] not in seen:
            seen.add(event['key'])
            events.append(event)
    return events


def find_exit_code(lines):
    """終了コードの目印を探し、見つかればその値を返す。"""
    for line in reversed(lines):
        match = EXIT_PATTERN.search(line)
        if match:
            return int(match.group(1))
    return None


def exit_code_finding(exit_code, mode_label='処理', has_error=False):
    """終了コードから、成功・失敗を伝える指摘を作る。

    has_error には、ログ中に error レベルの指摘があったかを渡す。
    終了コードが 0 でもエラーが記録されていることがあるため、
    その場合は「正常に終了しました」とは言わずに確認を促す。
    """
    if exit_code is None:
        return None
    if exit_code == 0:
        if has_error:
            return {
                'key': 'exit_ok_with_error',
                'level': 'warning',
                'title': f'{mode_label}は終了しましたが、エラーが記録されています',
                'detail': '処理自体は終了コード 0 で終わりましたが、'
                          '実行ログにエラーが出力されています。',
                'hint': '出力された地図やウェイポイントが正しいか確認してください。',
                'raw': '',
            }
        return {
            'key': 'exit_ok',
            'level': 'ok',
            'title': f'{mode_label}が正常に終了しました',
            'detail': '',
            'hint': '',
            'raw': '',
        }
    return {
        'key': 'exit_failed',
        'level': 'error',
        'title': f'{mode_label}が途中で終了しました',
        'detail': f'処理はエラー（終了コード {exit_code}）で終わりました。地図は作成されていません。',
        'hint': '上に表示されている原因と、下の実行ログを確認してください。',
        'raw': '',
    }
