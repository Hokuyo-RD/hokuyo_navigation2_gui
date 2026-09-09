"""マッピング設定（コンフィグCSV）と ROS Bag の食い違いを検査するモジュール。

マッピングが失敗する原因の多くは、コンフィグに書かれたトピック名や座標系名が
ROS Bag の中身と一致していないことにある。しかし実行するスクリプトは
「該当メッセージが0件だった」だけで静かに終わってしまうため、
非エンジニアには原因が分からない。

このモジュールは実行前に次を突き合わせ、日本語の指摘リストを返す。
  * コンフィグCSVの形式（bashスクリプトが行の並び順で値を読むため、順序が重要）
  * コンフィグのトピック名が ROS Bag に存在し、期待する型で、中身があるか
  * コンフィグの座標系名 (orig_frame / target_frame) が ROS Bag の
    オドメトリの frame_id / child_frame_id と一致するか

指摘は {level, category, title, detail, hint} の辞書で返す。
level は 'error'（このままでは失敗する） / 'warning'（失敗する可能性がある） /
'ok'（確認できた） / 'info'（参考情報）。
"""

import csv
import difflib
import os
import re

CONFIG_HEADER = ['オプション', '指定値', 'デフォルト値']

# マッピング用 bash スクリプト（hokuyo_slam.bash / lio_raw.bash / pcd2pgm.bash）は
# CSV を「行番号」で読み込むため、行の並び順そのものが仕様になっている。
# ここの並びは各スクリプトの option_arr[N] の割り当てと一致させること。
CONFIG_OPTION_ORDER = [
    'gnss_topic',
    'pointcloud_topic',
    'lio_topic',
    'run_lio',
    'gnss_cov_thre',
    'imu_topic',
    'slam_mode',
    'pc_save_distance',
    'wp_save_distance',
    'gnss_min_movement_thre',
    'lio_min_movement_thre',
    'gravity_stride',
    'orig_frame',
    'target_frame',
    'thre_z_min',
    'thre_z_max',
    'map_resolution',
    'thres_point_count',
    'flag_pass_through',
    'thre_radius',
    'waypoint_tolerance',
    'fix_rate',
]

# 設定ファイルを新規作成するときの雛形。(オプション, 指定値, デフォルト値)
# 「デフォルト値」の列には、各 bash スクリプトが内蔵している既定値を入れている。
#   gnss_topic〜gravity_stride, fix_rate : scripts/mapping/hokuyo_slam.bash
#   orig_frame, target_frame             : scripts/mapping/lio_raw.bash
#   thre_z_min〜waypoint_tolerance        : scripts/mapping/pcd2pgm.bash
# 並び順は CONFIG_OPTION_ORDER と必ず一致させること（スクリプトが行番号で読むため）。
CONFIG_TEMPLATE_ROWS = [
    ('gnss_topic', '/fix', '/fix'),
    ('pointcloud_topic', '/hokuyo3d/hokuyo_cloud2', '/hokuyo3d/hokuyo_cloud2'),
    ('lio_topic', '/rsf/lio_lidar_rate_odom', '/rsf/lio_lidar_rate_odom'),
    ('run_lio', 'true', 'true'),
    ('gnss_cov_thre', '0.1', '0.1'),
    ('imu_topic', '/imu/data', '/imu/data'),
    ('slam_mode', 'p2o', 'p2o'),
    ('pc_save_distance', '1.0', '1.0'),
    ('wp_save_distance', '4.0', '4.0'),
    ('gnss_min_movement_thre', '4.0', '4.0'),
    ('lio_min_movement_thre', '0.1', '0.1'),
    ('gravity_stride', '1', '1'),
    ('orig_frame', 'yvt', 'yvt'),
    ('target_frame', 'lio_odom', 'lio_odom'),
    ('thre_z_min', '-1.0', '-1.0'),
    ('thre_z_max', '20.0', '20.0'),
    ('map_resolution', '0.05', '0.05'),
    ('thres_point_count', '1', '1'),
    ('flag_pass_through', 'False', 'False'),
    ('thre_radius', '0.1', '0.1'),
    ('waypoint_tolerance', '1.0', '1.0'),
    ('fix_rate', '40', '40'),
]

# 各オプションの日本語ラベルと種別。種別は検査方法の切り替えに使う。
OPTION_INFO = {
    'gnss_topic':             ('GNSSトピック', 'topic'),
    'pointcloud_topic':       ('点群トピック', 'topic'),
    'lio_topic':              ('オドメトリ(LIO)トピック', 'topic'),
    'run_lio':                ('LIOを実行するか', 'bool'),
    'gnss_cov_thre':          ('GNSS共分散のしきい値', 'number'),
    'imu_topic':              ('IMUトピック', 'topic'),
    'slam_mode':              ('SLAMモード', 'choice'),
    'pc_save_distance':       ('点群を保存する間隔[m]', 'number'),
    'wp_save_distance':       ('ウェイポイントを保存する間隔[m]', 'number'),
    'gnss_min_movement_thre': ('GNSSの最小移動量[m]', 'number'),
    'lio_min_movement_thre':  ('LIOの最小移動量[m]', 'number'),
    'gravity_stride':         ('重力拘束の間引き数', 'number'),
    'orig_frame':             ('変換元の座標系(orig_frame)', 'frame'),
    'target_frame':           ('変換先の座標系(target_frame)', 'frame'),
    'thre_z_min':             ('地図化する高さの下限[m]', 'number'),
    'thre_z_max':             ('地図化する高さの上限[m]', 'number'),
    'map_resolution':         ('2D地図の解像度[m/pixel]', 'number'),
    'thres_point_count':      ('占有と判定する点数', 'number'),
    'flag_pass_through':      ('高さフィルタを使うか', 'bool'),
    'thre_radius':            ('外れ値除去の半径[m]', 'number'),
    'waypoint_tolerance':     ('ウェイポイント許容誤差[m]', 'number'),
    'fix_rate':               ('必要なGNSS Fix率[%]', 'number'),
}

# トピック種別のオプションに期待するメッセージ型。
EXPECTED_TOPIC_TYPES = {
    'gnss_topic': 'sensor_msgs/msg/NavSatFix',
    'pointcloud_topic': 'sensor_msgs/msg/PointCloud2',
    'lio_topic': 'nav_msgs/msg/Odometry',
    'imu_topic': 'sensor_msgs/msg/Imu',
}

VALID_SLAM_MODES = ('gnss', 'p2o', 'gravity')

# モードごとに、そのマッピングで必ず使われるトピックオプション。
REQUIRED_TOPICS = {
    'p2o': ['gnss_topic', 'pointcloud_topic', 'lio_topic'],
    'p2o_gravity': ['pointcloud_topic', 'lio_topic', 'imu_topic'],
    'lio_raw': ['pointcloud_topic', 'lio_topic'],
    'sync': ['pointcloud_topic', 'lio_topic'],
}

# モードごとに実行される bash スクリプト（存在確認に使う）。
MODE_SCRIPTS = {
    'p2o': 'mapping/hokuyo_slam.bash',
    'lio_raw': 'mapping/lio_raw.bash',
    'sync': 'mapping/sync_topic.bash',
    'pcd2pgm': 'mapping/pcd2pgm.bash',
}

MODE_LABELS = {
    'p2o': 'P2O マッピング',
    'lio_raw': 'LIO-RAW マッピング',
    'sync': 'トピック同期',
    'pcd2pgm': 'PCD→PGM 変換',
    'filter': 'トピックフィルタリング',
}


# 真偽値オプションで選べる値。
BOOL_CHOICES = ('true', 'false')


def _best_topic(bag_info, option, default_value):
    """そのオプションに一番ふさわしいトピックを ROS Bag から選ぶ。

    期待する型と一致し、かつデータが入っているものが候補。
    その中から「既定のトピック名にどれだけ名前が似ているか」を優先し、
    同程度ならメッセージ数が多いものを選ぶ。
    （例: 既定 /rsf/lio_lidar_rate_odom に対して /rsf/lio_imu_rate_odom を選ぶ）
    """
    expected_type = EXPECTED_TOPIC_TYPES.get(option)
    candidates = [t for t in bag_info.get('topics', [])
                  if t['type'] == expected_type and t['message_count'] > 0]
    if not candidates:
        return None
    def score(topic):
        similarity = difflib.SequenceMatcher(None, default_value, topic['name']).ratio()
        return (similarity, topic['message_count'])
    return max(candidates, key=score)['name']


def suggest_config_values(bag_info):
    """ROS Bag の内容から、設定値の初期値を決める。

    新規作成時に、トピック名と座標系をあらかじめその ROS Bag に合わせておく。
    決められなかった項目は戻り値に含めない（雛形の既定値がそのまま使われる）。
    """
    values = {}
    if not bag_info:
        return values

    defaults = {name: specified for name, specified, _default in CONFIG_TEMPLATE_ROWS}

    for option in EXPECTED_TOPIC_TYPES:
        name = _best_topic(bag_info, option, defaults.get(option, ''))
        if name:
            values[option] = name

    odom = _reference_odometry(bag_info, values)
    if odom:
        if odom.get('frame_id'):
            values['target_frame'] = odom['frame_id']
        if odom.get('child_frame_id'):
            values['orig_frame'] = odom['child_frame_id']

    return values


def mapping_config_template(bag_info=None):
    """新規作成用の設定CSVの中身を文字列で返す。

    bag_info を渡すと、トピック名と座標系をその ROS Bag に合わせて埋める。
    「デフォルト値」の列はスクリプトの既定値のまま変えない。
    """
    suggested = suggest_config_values(bag_info)
    lines = [','.join(CONFIG_HEADER)]
    for name, specified, default in CONFIG_TEMPLATE_ROWS:
        lines.append(','.join([name, suggested.get(name, specified), default]))
    return '\n'.join(lines) + '\n'


def option_kind(name):
    """オプションの種別（topic / frame / number / bool / choice / text）を返す。"""
    info = OPTION_INFO.get(name)
    return info[1] if info else 'text'


def _topic_choices(option, bag_info):
    """トピック名オプションの候補を、ROS Bag に実在するトピックから作る。

    期待する型と一致し、かつデータが入っているトピックを「おすすめ」として先頭に置く。
    """
    expected_type = EXPECTED_TOPIC_TYPES.get(option)
    choices = []
    for topic in bag_info.get('topics', []):
        label = topic['name']
        details = []
        if topic.get('type_label'):
            details.append(topic['type_label'])
        details.append(f'{topic["message_count"]}件')
        if topic.get('frame_id'):
            details.append(topic['frame_id'])
        recommended = bool(expected_type
                           and topic['type'] == expected_type
                           and topic['message_count'] > 0)
        choices.append({
            'value': topic['name'],
            'label': f'{label}（{" / ".join(details)}）',
            'recommended': recommended,
        })
    # おすすめを先頭に、その中ではトピック名順に並べる。
    choices.sort(key=lambda c: (not c['recommended'], c['value']))
    return choices


def _reference_odometry(bag_info, values):
    """座標系の候補を出すための基準になるオドメトリトピックを選ぶ。

    設定されている lio_topic を優先する。それが ROS Bag に無い場合
    （まさに設定を直したい場面）は、メッセージ数が最も多い
    オドメトリトピックで代用する。
    """
    odom = _topic_by_name(bag_info, (values or {}).get('lio_topic', ''))
    if odom and odom.get('frame_id'):
        return odom

    candidates = [t for t in bag_info.get('topics', [])
                  if t['type'] == EXPECTED_TOPIC_TYPES['lio_topic']
                  and t['message_count'] > 0 and t.get('frame_id')]
    if not candidates:
        return None
    return max(candidates, key=lambda t: t['message_count'])


def _frame_choices(option, bag_info, values):
    """座標系オプションの候補を、ROS Bag に出てくる座標系から作る。

    LIO-RAW はオドメトリの frame_id / child_frame_id と完全一致する必要があるため、
    その2つを「おすすめ」として示す。
    """
    odom = _reference_odometry(bag_info, values)
    if option == 'target_frame':
        recommended = (odom or {}).get('frame_id') or ''
    else:
        recommended = (odom or {}).get('child_frame_id') or ''

    choices = []
    for frame in bag_info.get('frames', []):
        label = frame
        if recommended and frame == recommended:
            label = f'{frame}（{odom["name"]} に合う）'
        choices.append({
            'value': frame,
            'label': label,
            'recommended': bool(recommended) and frame == recommended,
        })
    choices.sort(key=lambda c: (not c['recommended'], c['value']))
    return choices


def build_option_choices(bag_info, values=None):
    """各オプションのプルダウン候補をまとめて返す。

    候補を出せないオプション（数値など）は含めない。
    bag_info が無い場合は、ROS Bag に依存しない候補だけを返す。
    """
    choices = {
        'slam_mode': [{'value': mode, 'label': mode, 'recommended': False}
                      for mode in VALID_SLAM_MODES],
    }
    for name, (_label, kind) in OPTION_INFO.items():
        if kind == 'bool':
            choices[name] = [{'value': v, 'label': v, 'recommended': False}
                             for v in BOOL_CHOICES]

    if not bag_info:
        return choices

    for name, (_label, kind) in OPTION_INFO.items():
        if kind == 'topic':
            choices[name] = _topic_choices(name, bag_info)
        elif kind == 'frame':
            choices[name] = _frame_choices(name, bag_info, values)
    return choices


def _finding(level, category, title, detail='', hint=''):
    return {'level': level, 'category': category, 'title': title,
            'detail': detail, 'hint': hint}


def option_label(name):
    """オプション名の日本語ラベルを返す。未知の名前はそのまま返す。"""
    info = OPTION_INFO.get(name)
    return info[0] if info else name


def load_mapping_config(config_path):
    """コンフィグCSVを読み込み、値・行一覧・形式に関する指摘を返す。

    戻り値: {'path', 'name', 'values', 'rows', 'findings'}
    `values` は「指定値が空ならデフォルト値」を適用した後の値。
    """
    result = {'path': config_path, 'name': os.path.basename(config_path or ''),
              'values': {}, 'rows': [], 'findings': []}

    if not config_path:
        result['findings'].append(_finding(
            'warning', 'config',
            'パラメータ設定ファイルが選択されていません',
            'トピック名や座標系名は、スクリプトに埋め込まれた既定値が使われます。',
            'ROS Bag の内容に合わせた設定ファイル (CSV) を選択してください。'))
        return result

    if not os.path.isfile(config_path):
        result['findings'].append(_finding(
            'error', 'config',
            'パラメータ設定ファイルが見つかりません',
            f'指定されたファイルがありません: {config_path}',
            'ファイル管理画面で設定ファイル (CSV) を作り直すか、別のファイルを選び直してください。'))
        return result

    try:
        with open(config_path, 'r', encoding='utf-8', newline='') as f:
            rows = [row for row in csv.reader(f) if row and any(cell.strip() for cell in row)]
    except Exception as e:
        result['findings'].append(_finding(
            'error', 'config',
            'パラメータ設定ファイルを読み込めません',
            f'{config_path} の読み込みに失敗しました: {e}',
            'ファイルが文字コード UTF-8 の CSV になっているか確認してください。'))
        return result

    if not rows:
        result['findings'].append(_finding(
            'error', 'config', 'パラメータ設定ファイルが空です',
            f'{result["name"]} に1行も書かれていません。',
            '正しい設定ファイルを選び直してください。'))
        return result

    header = [cell.strip() for cell in rows[0]]
    if header[:3] != CONFIG_HEADER:
        result['findings'].append(_finding(
            'error', 'config',
            'パラメータ設定ファイルの見出し行が違います',
            f'1行目が「{",".join(header)}」になっています。'
            f'「{",".join(CONFIG_HEADER)}」である必要があります。',
            'マッピング用の設定ファイルではない可能性があります。別のファイルを選び直してください。'))
        return result

    data_rows = rows[1:]
    for index, row in enumerate(data_rows):
        name = row[0].strip() if len(row) > 0 else ''
        specified = row[1].strip() if len(row) > 1 else ''
        default = row[2].strip() if len(row) > 2 else ''
        value = specified or default
        result['rows'].append({
            'index': index,
            'name': name,
            'label': option_label(name),
            'specified': specified,
            'default': default,
            'value': value,
            'expected': CONFIG_OPTION_ORDER[index] if index < len(CONFIG_OPTION_ORDER) else '',
        })
        if name:
            result['values'][name] = value

    # マッピングのbashスクリプトはCSVを「行番号」で読むため、
    # 行を並べ替えたり削ったりすると、値が別のオプションに割り当てられてしまう。
    misplaced = [row for row in result['rows']
                 if row['expected'] and row['name'] != row['expected']]
    if misplaced:
        first = misplaced[0]
        result['findings'].append(_finding(
            'error', 'config',
            'パラメータ設定ファイルの行の並び順が違います',
            f'{first["index"] + 2}行目は「{first["expected"]}」でなければいけませんが、'
            f'「{first["name"] or "（空欄）"}」になっています。'
            f'（ずれている行: {len(misplaced)}行）',
            'マッピングの実行スクリプトは行の順番で値を読み取ります。'
            '行を並べ替えたり削除したりせず、既定の設定ファイルをコピーして値だけを書き換えてください。'))
    elif len(result['rows']) < len(CONFIG_OPTION_ORDER):
        missing = CONFIG_OPTION_ORDER[len(result['rows']):]
        result['findings'].append(_finding(
            'warning', 'config',
            'パラメータ設定ファイルの行が足りません',
            f'{len(CONFIG_OPTION_ORDER)}行必要ですが、{len(result["rows"])}行しかありません。'
            f'不足している項目: {", ".join(missing)}',
            '不足している項目はスクリプトの既定値で動作します。'
            '意図した設定にしたい場合は行を追加してください。'))

    for row in result['rows']:
        if row['name'] and not row['value']:
            result['findings'].append(_finding(
                'warning', 'config',
                f'{row["label"]} の値が空欄です',
                f'{row["index"] + 2}行目「{row["name"]}」に指定値もデフォルト値もありません。',
                'スクリプトの既定値が使われます。意図した値を入力してください。'))

    return result


# マッピングの bash スクリプトは CSV を `cut -d ',' -f 2` で読み、
# 値を空白で区切って配列に入れる。そのためカンマや空白を含む値は書き込めない。
_INVALID_VALUE_PATTERN = re.compile(r'[,\s"]')


def validate_config_value(name, value):
    """設定値として書き込める文字列かを調べる。問題があれば理由を返す。"""
    if _INVALID_VALUE_PATTERN.search(value):
        return (f'{option_label(name)} に空白・カンマ・引用符は使えません。'
                '（マッピングのスクリプトが値を読み取れなくなります）')
    if len(value) > 200:
        return f'{option_label(name)} の値が長すぎます。'
    return None


def write_mapping_config(source_path, values, target_path):
    """設定CSVの「指定値」だけを書き換えて保存する。

    行の並び順とデフォルト値の列はそのまま残す。マッピングのスクリプトは
    CSV を行番号で読むため、行を増減・並べ替えしてはいけない。

    戻り値: (成功したか, エラーメッセージの一覧)
    """
    errors = []
    for name, value in values.items():
        message = validate_config_value(name, value)
        if message:
            errors.append(message)
    if errors:
        return False, errors

    try:
        with open(source_path, 'r', encoding='utf-8', newline='') as f:
            rows = [row for row in csv.reader(f) if row and any(cell.strip() for cell in row)]
    except OSError as e:
        return False, [f'設定ファイルを読み込めませんでした: {e}']

    if not rows or [cell.strip() for cell in rows[0]][:3] != CONFIG_HEADER:
        return False, ['マッピング用の設定ファイルではありません。']

    output = [list(CONFIG_HEADER)]
    for row in rows[1:]:
        name = row[0].strip() if row else ''
        specified = row[1].strip() if len(row) > 1 else ''
        if name in values:
            specified = values[name]
        # デフォルト値の列が元から無い行は、無いまま書き戻す。
        # 触っていない行の見た目を変えないため。
        if len(row) > 2:
            output.append([name, specified, row[2].strip()])
        else:
            output.append([name, specified])

    try:
        with open(target_path, 'w', encoding='utf-8', newline='') as f:
            csv.writer(f, lineterminator='\n').writerows(output)
    except OSError as e:
        return False, [f'設定ファイルを保存できませんでした: {e}']

    return True, []


def _check_number(name, value, findings):
    try:
        float(value)
    except (TypeError, ValueError):
        findings.append(_finding(
            'error', 'config',
            f'{option_label(name)} が数値ではありません',
            f'「{name}」に「{value}」が指定されています。',
            '数値（例: 0.05）を入力してください。'))


def _topic_by_name(bag_info, topic_name):
    for topic in bag_info.get('topics', []):
        if topic['name'] == topic_name:
            return topic
    return None


def _check_topic_option(option, topic_name, bag_info, required, findings):
    """コンフィグのトピック名が ROS Bag の中身と合っているか調べる。"""
    label = option_label(option)
    level = 'error' if required else 'warning'

    if not topic_name:
        findings.append(_finding(
            level, 'topic', f'{label}が設定されていません',
            f'コンフィグの「{option}」が空欄です。',
            'ROS Bag に記録されているトピック名を入力してください。'))
        return

    topic = _topic_by_name(bag_info, topic_name)
    if topic is None:
        names = [t['name'] for t in bag_info.get('topics', [])]
        expected_type = EXPECTED_TOPIC_TYPES.get(option)
        # 型が一致するトピックを優先して提案し、無ければ名前が似たものを提案する。
        same_type = [t['name'] for t in bag_info.get('topics', [])
                     if t['type'] == expected_type and t['message_count'] > 0]
        suggestions = same_type or difflib.get_close_matches(topic_name, names, n=3, cutoff=0.4)
        hint = f'ROS Bag に合わせて「{option}」を修正してください。'
        if suggestions:
            hint = (f'このROS Bagには {", ".join(suggestions)} が記録されています。'
                    f'「{option}」をこのいずれかに書き換えてください。')
        findings.append(_finding(
            level, 'topic',
            f'{label}「{topic_name}」がROS Bagにありません',
            f'選択したROS Bagには「{topic_name}」というトピックが記録されていません。',
            hint))
        return

    if topic['message_count'] == 0:
        expected_type = EXPECTED_TOPIC_TYPES.get(option)
        alternatives = [t['name'] for t in bag_info.get('topics', [])
                        if t['type'] == expected_type and t['message_count'] > 0]
        hint = 'センサやノードが動いていない状態で記録された可能性があります。データ取得をやり直してください。'
        if alternatives:
            hint = (f'中身のある同じ種類のトピックがあります: {", ".join(alternatives)}。'
                    f'「{option}」をこのいずれかに書き換えてください。')
        findings.append(_finding(
            level, 'topic',
            f'{label}「{topic_name}」にデータが入っていません',
            'トピックは存在しますが、メッセージが0件です。このままでは地図が作られません。',
            hint))
        return

    expected_type = EXPECTED_TOPIC_TYPES.get(option)
    if expected_type and topic['type'] != expected_type:
        findings.append(_finding(
            'error', 'topic',
            f'{label}「{topic_name}」の種類が違います',
            f'{expected_type} が必要ですが、このトピックは {topic["type"]} です。',
            f'「{option}」に正しいトピック名を指定してください。'))
        return

    findings.append(_finding(
        'ok', 'topic',
        f'{label}「{topic_name}」を確認しました',
        f'{topic["message_count"]}件（約{topic["frequency"]}Hz）'
        + (f' / 座標系: {topic["frame_id"]}' if topic.get('frame_id') else ''),
        ''))


def _check_frames(config_values, bag_info, findings):
    """orig_frame / target_frame が ROS Bag の座標系と一致するか調べる。

    LIO-RAW マッピングは、オドメトリの header.frame_id が target_frame と、
    child_frame_id が orig_frame と完全一致した場合だけ点群を積み上げる
    (src/pcd_tf_extractor.py)。一致しないと、エラーを出さずに空の地図ができる。
    """
    orig_frame = config_values.get('orig_frame', '')
    target_frame = config_values.get('target_frame', '')
    lio_topic = config_values.get('lio_topic', '')
    pointcloud_topic = config_values.get('pointcloud_topic', '')

    odom = _topic_by_name(bag_info, lio_topic)
    known_frames = bag_info.get('frames', [])

    if odom is None or not odom.get('frame_id'):
        findings.append(_finding(
            'warning', 'frame',
            '座標系の照合ができませんでした',
            f'オドメトリトピック「{lio_topic}」の座標系を読み取れなかったため、'
            'orig_frame / target_frame が正しいか確認できませんでした。',
            '先にオドメトリトピックの設定を直してから、もう一度確認してください。'))
        return

    actual_target = odom.get('frame_id') or ''
    actual_orig = odom.get('child_frame_id') or ''

    if target_frame and actual_target and target_frame != actual_target:
        findings.append(_finding(
            'error', 'frame',
            f'target_frame「{target_frame}」がROS Bagと一致しません',
            f'オドメトリ「{lio_topic}」が使っている座標系は「{actual_target}」です。',
            f'コンフィグの target_frame を「{actual_target}」に変更してください。'
            '一致しないと、エラーが出ないまま中身が空の地図が作られます。'))
    elif target_frame and target_frame == actual_target:
        findings.append(_finding(
            'ok', 'frame',
            f'target_frame「{target_frame}」を確認しました',
            f'オドメトリ「{lio_topic}」の座標系と一致しています。', ''))

    if orig_frame and actual_orig and orig_frame != actual_orig:
        findings.append(_finding(
            'error', 'frame',
            f'orig_frame「{orig_frame}」がROS Bagと一致しません',
            f'オドメトリ「{lio_topic}」が指しているロボット側の座標系は「{actual_orig}」です。',
            f'コンフィグの orig_frame を「{actual_orig}」に変更してください。'
            '一致しないと、エラーが出ないまま中身が空の地図が作られます。'))
    elif orig_frame and orig_frame == actual_orig:
        findings.append(_finding(
            'ok', 'frame',
            f'orig_frame「{orig_frame}」を確認しました',
            f'オドメトリ「{lio_topic}」の子座標系と一致しています。', ''))
    elif orig_frame and not actual_orig:
        findings.append(_finding(
            'warning', 'frame',
            'オドメトリに子座標系(child_frame_id)が記録されていません',
            f'「{lio_topic}」の child_frame_id が空です。'
            f'コンフィグの orig_frame は「{orig_frame}」です。',
            'このROS Bagでは座標系の照合ができません。'
            'LIO-RAW マッピングで空の地図ができる場合は、データ取得をやり直してください。'))

    pointcloud = _topic_by_name(bag_info, pointcloud_topic)
    if pointcloud and pointcloud.get('frame_id') and orig_frame \
            and pointcloud['frame_id'] != orig_frame:
        findings.append(_finding(
            'warning', 'frame',
            '点群とorig_frameの座標系が違います',
            f'点群「{pointcloud_topic}」の座標系は「{pointcloud["frame_id"]}」ですが、'
            f'コンフィグの orig_frame は「{orig_frame}」です。',
            'LIO-RAW マッピングは両者が同じ位置を指している前提で動きます。'
            '地図が二重に見えたりずれたりする場合は、この設定を見直してください。'))

    for option in ('orig_frame', 'target_frame'):
        name = config_values.get(option, '')
        if name and known_frames and name not in known_frames:
            findings.append(_finding(
                'info', 'frame',
                f'{option}「{name}」はROS Bagに出てきません',
                f'このROS Bagで使われている座標系: {", ".join(known_frames)}',
                'この一覧にある名前から選ぶのが確実です。'))


def diagnose(mode, config_path, bag_info, scripts_dir=None):
    """マッピング実行前の検査を行い、指摘の一覧と集計を返す。

    mode: 'p2o' / 'lio_raw' / 'sync' / 'pcd2pgm' / 'filter'
    config_path: 選択されたコンフィグCSVのパス（未選択なら空文字）
    bag_info: rosbag_inspect.inspect_bag() の戻り値（無ければ None）
    scripts_dir: hokuyo_navigation2 の scripts ディレクトリ（実行スクリプトの存在確認用）
    """
    config = load_mapping_config(config_path)
    findings = list(config['findings'])
    values = config['values']

    # 実行スクリプトそのものが無ければ、何を設定しても処理は始まらない。
    script_rel = MODE_SCRIPTS.get(mode)
    if scripts_dir and script_rel:
        script_path = os.path.join(scripts_dir, script_rel)
        if not os.path.isfile(script_path):
            findings.append(_finding(
                'error', 'env',
                f'{MODE_LABELS.get(mode, mode)} の実行スクリプトがありません',
                f'{script_path} が見つかりません。'
                'この状態で実行しても、処理は何も行われずに終了します。',
                'hokuyo_navigation2 パッケージが正しく取得・更新されているか、'
                'システム管理者に確認してください。'))

    for name in ('gnss_cov_thre', 'pc_save_distance', 'wp_save_distance',
                 'gnss_min_movement_thre', 'lio_min_movement_thre', 'gravity_stride',
                 'thre_z_min', 'thre_z_max', 'map_resolution', 'thres_point_count',
                 'thre_radius', 'waypoint_tolerance', 'fix_rate'):
        if name in values and values[name]:
            _check_number(name, values[name], findings)

    if values.get('thre_z_min') and values.get('thre_z_max'):
        try:
            if float(values['thre_z_min']) >= float(values['thre_z_max']):
                findings.append(_finding(
                    'error', 'config',
                    '地図化する高さの範囲が逆になっています',
                    f'下限 {values["thre_z_min"]} が上限 {values["thre_z_max"]} 以上です。',
                    '下限 (thre_z_min) には上限 (thre_z_max) より小さい値を入れてください。'))
        except ValueError:
            pass

    slam_mode = values.get('slam_mode', '')
    if slam_mode and slam_mode not in VALID_SLAM_MODES:
        findings.append(_finding(
            'error', 'config',
            f'SLAMモード「{slam_mode}」は使えません',
            f'指定できるのは {", ".join(VALID_SLAM_MODES)} のいずれかです。',
            'コンフィグの slam_mode を書き直してください。'))

    # 設定ファイルが選ばれていない場合は、スクリプト内蔵の既定値で動く。
    # 何が使われるか分からないため、トピックや座標系の照合は行わない。
    has_config = bool(values)

    if mode in ('p2o', 'lio_raw', 'sync') and bag_info and has_config:
        required_key = mode
        if mode == 'p2o' and slam_mode == 'gravity':
            required_key = 'p2o_gravity'
        required = REQUIRED_TOPICS.get(required_key, [])
        # 必須ではないトピックも、設定されていれば併せて確認する。
        optional = [opt for opt in ('gnss_topic', 'pointcloud_topic', 'lio_topic', 'imu_topic')
                    if opt not in required and values.get(opt)]
        for option in required:
            _check_topic_option(option, values.get(option, ''), bag_info, True, findings)
        for option in optional:
            _check_topic_option(option, values.get(option, ''), bag_info, False, findings)

    if mode == 'lio_raw' and bag_info and has_config:
        _check_frames(values, bag_info, findings)

    if bag_info and not bag_info.get('topics'):
        findings.append(_finding(
            'error', 'bag',
            '選択したROS Bagにトピックがありません',
            'ROS Bag は開けましたが、トピックが1件も記録されていません。',
            '別のROS Bagを選び直してください。'))

    summary = {'error': 0, 'warning': 0, 'ok': 0, 'info': 0}
    for finding in findings:
        summary[finding['level']] = summary.get(finding['level'], 0) + 1

    if summary['error']:
        status = 'error'
        message = (f'このまま実行すると失敗する可能性が高い問題が {summary["error"]} 件あります。'
                   '下の内容を確認してください。')
    elif not has_config:
        status = 'warning'
        message = ('パラメータ設定ファイルが選ばれていないため、ROS Bagとの照合はできていません。'
                   '設定ファイルを選ぶと内容を確認できます。')
    elif summary['warning']:
        status = 'warning'
        message = f'確認したほうがよい点が {summary["warning"]} 件あります。'
    else:
        status = 'ok'
        message = '設定とROS Bagの内容に、目立った食い違いはありません。'

    # 深刻なものから順に並べる。
    order = {'error': 0, 'warning': 1, 'info': 2, 'ok': 3}
    findings.sort(key=lambda f: order.get(f['level'], 9))

    return {
        'mode': mode,
        'mode_label': MODE_LABELS.get(mode, mode),
        'config_name': config['name'],
        'config_rows': config['rows'],
        'findings': findings,
        'summary': summary,
        'status': status,
        'message': message,
    }
