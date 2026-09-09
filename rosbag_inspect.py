"""ROS Bag の中身を調べて、ブラウザGUIに表示できる形に整えるモジュール。

`rosbag2_filter_core` がトピック名と型だけを返すのに対し、こちらは
「そのROS Bagで本当にマッピングできるのか」を人が判断するために必要な情報
（メッセージ数・周波数・記録時間・各トピックの座標系(frame_id)・TFツリー）
をまとめて取得する。取得した情報は `mapping_diagnostics` でコンフィグの
検査にも使われる。

ROS Bag 全体を読み直すと巨大なBagでは時間が掛かるため、
  * メタデータ (トピック一覧・件数・時間) は metadata から取得する
  * frame_id は各トピックの先頭メッセージだけを覗き見る
  * TF は /tf, /tf_static だけに絞って先頭部分を読む
という方針で、必ず上限を設けて読み込む。
"""

import datetime
import os
import struct

from rosbag2_py import SequentialReader, StorageOptions, ConverterOptions, StorageFilter
from rosidl_runtime_py.utilities import get_message

# ROS Bag の URI とストレージ種別の判定は rosbag2_filter_core と同じ規則を使う。
# 判定を二重に持つと、対応形式を増やしたときに片方だけ直し忘れる。
from rosbag2_filter_core import _get_bag_info

# --- 読み込み量の上限 ------------------------------------------------------
# frame_id は先頭メッセージが分かれば十分なので、全トピック分が揃った時点で
# 打ち切る。揃わない場合でも下記の上限で必ず読み込みを止める。
MAX_HEADER_MESSAGES = 2000
MAX_HEADER_BYTES = 256 * 1024 * 1024
# TF は同じ組み合わせが繰り返し記録されるため、先頭の一定件数で十分。
MAX_TF_MESSAGES = 3000

TF_TOPIC_TYPE = 'tf2_msgs/msg/TFMessage'

# ROS Bag に記録されうる代表的な型の、日本語での説明。
# 非エンジニアが「このトピックが何なのか」を判断できるようにするために使う。
TYPE_DESCRIPTIONS = {
    'sensor_msgs/msg/PointCloud2': '3D点群（LiDARの計測データ）',
    'sensor_msgs/msg/Imu': 'IMU（加速度・角速度）',
    'sensor_msgs/msg/NavSatFix': 'GNSS測位結果（緯度・経度・高度）',
    'sensor_msgs/msg/LaserScan': '2Dスキャン',
    'sensor_msgs/msg/Image': 'カメラ画像',
    'sensor_msgs/msg/CompressedImage': '圧縮カメラ画像',
    'nav_msgs/msg/Odometry': 'オドメトリ（自己位置の推定値）',
    'nav_msgs/msg/Path': '経路',
    'geometry_msgs/msg/Twist': '速度指令',
    'geometry_msgs/msg/PoseStamped': '位置姿勢',
    'geometry_msgs/msg/PoseWithCovarianceStamped': '位置姿勢（共分散付き）',
    'tf2_msgs/msg/TFMessage': '座標変換（TF）',
    'diagnostic_msgs/msg/DiagnosticArray': '診断情報',
    'nmea_msgs/msg/Gpgga': 'NMEA GGA センテンス',
    'nmea_msgs/msg/Gprmc': 'NMEA RMC センテンス',
    'nmea_msgs/msg/Gpzda': 'NMEA ZDA センテンス',
    'rosgraph_msgs/msg/Clock': '時刻',
    'std_msgs/msg/String': '文字列',
}


def describe_type(type_name):
    """メッセージ型の日本語説明を返す。未知の型は空文字。"""
    return TYPE_DESCRIPTIONS.get(type_name, '')


def format_duration(seconds):
    """秒数を「1分23秒」のような読みやすい文字列にする。"""
    if not seconds or seconds < 0:
        return '不明'
    seconds = float(seconds)
    if seconds < 60:
        return f'{seconds:.1f}秒'
    minutes, sec = divmod(int(round(seconds)), 60)
    hours, minutes = divmod(minutes, 60)
    if hours:
        return f'{hours}時間{minutes}分{sec}秒'
    return f'{minutes}分{sec}秒'


def format_size(num_bytes):
    """バイト数を MB / GB 表記にする。"""
    if not num_bytes:
        return '不明'
    size = float(num_bytes)
    for unit in ('B', 'KB', 'MB', 'GB', 'TB'):
        if size < 1024 or unit == 'TB':
            return f'{size:.1f} {unit}' if unit != 'B' else f'{int(size)} B'
        size /= 1024
    return f'{size:.1f} TB'


def _format_time(nanoseconds):
    if not nanoseconds:
        return ''
    try:
        stamp = datetime.datetime.fromtimestamp(nanoseconds / 1_000_000_000)
    except (ValueError, OSError, OverflowError):
        return ''
    return stamp.strftime('%Y-%m-%d %H:%M:%S')


def _open_reader(bag_path):
    """ROS Bag を開いた SequentialReader を返す。"""
    uri, storage_id, error_msg = _get_bag_info(bag_path)
    if error_msg:
        raise ValueError(error_msg)
    reader = SequentialReader()
    reader.open(StorageOptions(uri=uri, storage_id=storage_id),
                ConverterOptions())
    return reader


def _header_field_layout(type_name):
    """型の先頭が std_msgs/Header かどうかと、child_frame_id を持つかを返す。

    戻り値: (先頭がHeaderか, child_frame_idが続くか)
    型が読み込めない場合は (False, False)。
    """
    try:
        msg_cls = get_message(type_name)
    except Exception:
        return False, False
    fields = getattr(msg_cls, '_fields_and_field_types', None)
    if not fields:
        return False, False
    names = list(fields.keys())
    if not names or names[0] != 'header' or fields['header'] != 'std_msgs/Header':
        return False, False
    has_child = len(names) > 1 and names[1] == 'child_frame_id' and fields[names[1]] == 'string'
    return True, has_child


def _read_cdr_string(data, offset, little_endian):
    """CDR でシリアライズされた文字列を読み、(文字列, 次のオフセット) を返す。"""
    fmt = '<I' if little_endian else '>I'
    if offset + 4 > len(data):
        return None, offset
    (length,) = struct.unpack_from(fmt, data, offset)
    start = offset + 4
    end = start + length
    if length == 0 or end > len(data) or length > 1024:
        return None, end
    # ROS の文字列は終端の NUL を含めた長さで記録される。
    text = bytes(data[start:end - 1]).decode('utf-8', errors='replace')
    # 次のフィールドは 4 バイト境界に整列する（4バイトのカプセル化ヘッダは除く）。
    padding = (-(end - 4)) % 4
    return text, end + padding


def _peek_frames(data, has_child_frame):
    """生のCDRデータから frame_id（と child_frame_id）だけを取り出す。

    点群のような巨大なメッセージを丸ごとデシリアライズせずに済ませるため、
    先頭の std_msgs/Header だけをバイト列から直接読む。
    レイアウトは [4B カプセル化ヘッダ][int32 sec][uint32 nanosec][string frame_id]。
    """
    if len(data) < 16:
        return None, None
    little_endian = bool(data[1] & 0x01)
    frame_id, offset = _read_cdr_string(data, 12, little_endian)
    if frame_id is None:
        return None, None
    child_frame_id = None
    if has_child_frame:
        child_frame_id, _ = _read_cdr_string(data, offset, little_endian)
    return frame_id, child_frame_id


def _collect_tf_frames(bag_path, tf_topics):
    """TF トピックごとに、親子フレームの組み合わせを収集する。

    戻り値: {トピック名: [{'parent': ..., 'child': ...}, ...]}
    同じ組み合わせが繰り返し記録されるため、重複は取り除く。
    """
    edges = {topic: [] for topic in tf_topics}
    if not tf_topics:
        return edges

    from rclpy.serialization import deserialize_message
    try:
        tf_msg_cls = get_message(TF_TOPIC_TYPE)
    except Exception:
        return edges

    reader = _open_reader(bag_path)
    try:
        reader.set_filter(StorageFilter(topics=list(tf_topics)))
        seen = {topic: set() for topic in tf_topics}
        count = 0
        while reader.has_next() and count < MAX_TF_MESSAGES:
            topic, data, _ = reader.read_next()
            count += 1
            if topic not in edges:
                continue
            try:
                msg = deserialize_message(bytes(data), tf_msg_cls)
            except Exception:
                continue
            for transform in msg.transforms:
                pair = (transform.header.frame_id, transform.child_frame_id)
                if pair not in seen[topic]:
                    seen[topic].add(pair)
                    edges[topic].append({'parent': pair[0], 'child': pair[1]})
    finally:
        try:
            reader.close()
        except Exception:
            pass
    return edges


def _collect_frame_ids(bag_path, header_topics):
    """ヘッダを持つ各トピックの先頭メッセージから frame_id を集める。"""
    frames = {}
    if not header_topics:
        return frames

    reader = _open_reader(bag_path)
    try:
        reader.set_filter(StorageFilter(topics=list(header_topics.keys())))
        count = 0
        read_bytes = 0
        while reader.has_next():
            if count >= MAX_HEADER_MESSAGES or read_bytes >= MAX_HEADER_BYTES:
                break
            topic, data, _ = reader.read_next()
            count += 1
            read_bytes += len(data)
            if topic in frames:
                continue
            has_child = header_topics.get(topic, False)
            frame_id, child_frame_id = _peek_frames(data, has_child)
            frames[topic] = {'frame_id': frame_id, 'child_frame_id': child_frame_id}
            if len(frames) >= len(header_topics):
                break
    finally:
        try:
            reader.close()
        except Exception:
            pass
    return frames


def inspect_bag(bag_path):
    """ROS Bag を調べて、GUI表示と診断に使う情報一式を返す。

    戻り値の辞書:
      path, name, storage_id, size, size_bytes, duration_sec, duration,
      start_time, end_time, message_count, topics[], tf{}, frames[], warnings[]
    """
    bag_path = os.path.abspath(bag_path)
    reader = _open_reader(bag_path)
    try:
        metadata = reader.get_metadata()
    finally:
        try:
            reader.close()
        except Exception:
            pass

    duration_sec = 0.0
    try:
        duration_sec = metadata.duration.nanoseconds / 1_000_000_000
    except AttributeError:
        pass
    start_ns = 0
    try:
        start_ns = metadata.starting_time.nanoseconds
    except AttributeError:
        pass

    warnings = []
    topics = []
    header_topics = {}
    tf_topics = []

    for entry in metadata.topics_with_message_count:
        meta = entry.topic_metadata
        count = entry.message_count
        if meta.type == TF_TOPIC_TYPE:
            tf_topics.append(meta.name)
        has_header, has_child = _header_field_layout(meta.type)
        if has_header and count > 0:
            header_topics[meta.name] = has_child
        frequency = (count / duration_sec) if duration_sec > 0 and count else 0.0
        topics.append({
            'name': meta.name,
            'type': meta.type,
            'type_label': describe_type(meta.type),
            'message_count': count,
            'frequency': round(frequency, 1),
            'serialization_format': meta.serialization_format,
            'frame_id': None,
            'child_frame_id': None,
            'is_empty': count == 0,
        })
        if count == 0:
            warnings.append(f'トピック "{meta.name}" にメッセージが1件も記録されていません。')

    try:
        frame_ids = _collect_frame_ids(bag_path, header_topics)
    except Exception as e:
        frame_ids = {}
        warnings.append(f'座標系(frame_id)の読み取りに失敗しました: {e}')

    for topic in topics:
        info = frame_ids.get(topic['name'])
        if info:
            topic['frame_id'] = info.get('frame_id')
            topic['child_frame_id'] = info.get('child_frame_id')

    try:
        tf_edges = _collect_tf_frames(bag_path, tf_topics)
    except Exception as e:
        tf_edges = {}
        warnings.append(f'TF（座標変換）の読み取りに失敗しました: {e}')

    # 画面に「このBagに存在する座標系」を一覧するための集合。
    all_frames = set()
    for edges in tf_edges.values():
        for edge in edges:
            all_frames.add(edge['parent'])
            all_frames.add(edge['child'])
    for topic in topics:
        for key in ('frame_id', 'child_frame_id'):
            if topic[key]:
                all_frames.add(topic[key])

    topics.sort(key=lambda t: t['name'])

    return {
        'path': bag_path,
        'name': os.path.basename(bag_path.rstrip('/')),
        'storage_id': metadata.storage_identifier,
        'size_bytes': getattr(metadata, 'bag_size', 0),
        'size': format_size(getattr(metadata, 'bag_size', 0)),
        'duration_sec': duration_sec,
        'duration': format_duration(duration_sec),
        'start_time': _format_time(start_ns),
        'end_time': _format_time(start_ns + int(duration_sec * 1_000_000_000)) if start_ns else '',
        'message_count': metadata.message_count,
        'topic_count': len(topics),
        'topics': topics,
        'tf': tf_edges,
        'frames': sorted(all_frames),
        'warnings': warnings,
    }
