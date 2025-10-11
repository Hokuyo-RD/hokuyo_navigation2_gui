# rosbag2_filter_core.py
import os
import glob
import shutil
from rosbag2_py import SequentialReader, SequentialWriter, StorageOptions, ConverterOptions, TopicMetadata
from rosidl_runtime_py.utilities import get_message

# ROS 2 bagファイルのパスとストレージIDを決定するヘルパー関数
def _get_bag_info(bag_file_path):
    """
    与えられたパスからrosbagのURIとstorage_idを決定する。
    """
    input_uri_for_reader = None
    input_storage_id = None
    
    # パスがディレクトリの場合
    if os.path.isdir(bag_file_path):
        db3_files = glob.glob(os.path.join(bag_file_path, '*.db3'))
        mcap_files = glob.glob(os.path.join(bag_file_path, '*.mcap'))
        if db3_files:
            input_uri_for_reader = bag_file_path
            input_storage_id = 'sqlite3'
        elif mcap_files:
            input_uri_for_reader = bag_file_path
            input_storage_id = 'mcap'
        else:
            return None, None, "No .db3 or .mcap files found in bag directory."
    # パスが単一ファイルの場合
    else:
        # URIとして親ディレクトリを使用する (ROS 2の標準的なアプローチ)
        input_uri_for_reader = os.path.dirname(bag_file_path)
        if not input_uri_for_reader: input_uri_for_reader = "."
        
        # ファイル拡張子からストレージIDを決定
        if bag_file_path.endswith('.db3'):
            input_storage_id = 'sqlite3'
        elif bag_file_path.endswith('.mcap'):
            input_storage_id = 'mcap'
        elif bag_file_path.endswith('.bag'):
            # ROS 2では.bag拡張子もsqlite3として扱うことが多い
            input_storage_id = 'sqlite3' 
        else:
            return None, None, "Unsupported bag file extension."
            
    return input_uri_for_reader, input_storage_id, None


def get_topic_list(bag_file_path):
    """
    指定されたROS 2 bagファイル内のトピック名とタイプを取得します。
    """
    input_uri_for_reader, input_storage_id, error_msg = _get_bag_info(bag_file_path)
    if error_msg:
        raise ValueError(error_msg)

    reader = SequentialReader()
    try:
        storage_options = StorageOptions(uri=input_uri_for_reader, storage_id=input_storage_id)
        converter_options = ConverterOptions()

        reader.open(storage_options, converter_options)
        topic_types_info = reader.get_all_topics_and_types()
        
        topics_info = {}
        for info in topic_types_info:
            topics_info[info.name] = info.type

        return topics_info
    except Exception as e:
        raise RuntimeError(f"Failed to read rosbag2 file '{bag_file_path}': No storage could be initialized from the inputs. Check ROS 2 environment source.\nOriginal Error: {e}")
    finally:
        if 'reader' in locals() and reader:
            try: reader.close()
            except: pass


def filter_rosbag(input_bag_path, output_bag_folder_path, keeping_topics):
    """
    指定されたトピックのみを保持して新しいrosbagファイルを作成します。
    """
    if not keeping_topics:
        raise ValueError("Please select at least one topic to keep.")

    input_uri_for_reader, input_storage_id, error_msg = _get_bag_info(input_bag_path)
    if error_msg:
        raise ValueError(f"Input bag error: {error_msg}")
        
    # 出力パスのディレクトリチェック
    if os.path.exists(output_bag_folder_path):
        if os.path.isdir(output_bag_folder_path) and os.listdir(output_bag_folder_path):
            raise FileExistsError(f"Output bag directory '{output_bag_folder_path}' already exists and is not empty.")
        elif not os.path.isdir(output_bag_folder_path):
             os.remove(output_bag_folder_path)
             os.makedirs(output_bag_folder_path, exist_ok=True)
    else:
        os.makedirs(output_bag_folder_path, exist_ok=True)


    reader = SequentialReader()
    writer = SequentialWriter()
    
    try:
        # 読み込み設定
        storage_options_read = StorageOptions(uri=input_uri_for_reader, storage_id=input_storage_id)
        converter_options_read = ConverterOptions()
        reader.open(storage_options_read, converter_options_read)
        
        # 書き込み設定
        storage_options_write = StorageOptions(uri=output_bag_folder_path, storage_id='sqlite3')
        converter_options_write = ConverterOptions()
        writer.open(storage_options_write, converter_options_write)
        
        # トピック情報
        topic_types_info = reader.get_all_topics_and_types()
        topic_names_to_types = {info.name: info.type for info in topic_types_info}
        topics_to_filter = set(keeping_topics)
        
        # トピックの登録
        for topic_name in topics_to_filter.copy():
            msg_type_str = topic_names_to_types.get(topic_name)
            if msg_type_str:
                try:
                    get_message(msg_type_str) 
                    
                    topic_metadata = TopicMetadata(
                        name=topic_name,
                        type=msg_type_str,
                        serialization_format='cdr',
                        offered_qos_profiles="" 
                    )
                    writer.create_topic(topic_metadata)
                except Exception as e:
                    print(f"Warning: Could not load message type {msg_type_str} for topic {topic_name}: {e}. Skipping.")
                    topics_to_filter.discard(topic_name)
            else:
                print(f"Warning: No type found for topic {topic_name}. Skipping.")
                topics_to_filter.discard(topic_name)

        # メッセージの書き込み
        count = 0
        while reader.has_next():
            (topic_name, data, timestamp) = reader.read_next()
            if topic_name in topics_to_filter:
                writer.write(topic_name, data, timestamp)
                count += 1
        
        return f"Conversion finished. Wrote {count} messages."

    except Exception as e:
        raise RuntimeError(f"Conversion failed: {e}")
    finally:
        if 'reader' in locals() and reader:
            try: reader.close()
            except: pass
        if 'writer' in locals() and writer:
            try: writer.close()
            except: pass