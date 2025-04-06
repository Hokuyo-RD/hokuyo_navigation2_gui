#!/bin/bash

# rosbag record -a をバックグラウンドで実行
echo "rosbag record -a を開始します..."
rosbag record -a &
record_pid=$!
echo "rosbag record の PID: $record_pid"

# zenity で停止確認
if zenity --question --title="ROS Bag Record 停止確認" --text="rosbag record を停止しますか？" 2>/dev/null; then
  echo "OK が選択されました。rosbag record を停止します..."

  # PID を使って rosbag record を停止
  if [ -n "$record_pid" ]; then
    echo "PID $record_pid を kill します..."
    kill "$record_pid"
    echo "rosbag record を停止しました。"
  else
    echo "rosbag record の PID が不明です。"
  fi
else
  echo "キャンセルが選択されました。rosbag record は継続します。"
fi

EXITCODE=$?
echo "EXITCODE=$EXITCODE"