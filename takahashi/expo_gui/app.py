import os
import subprocess
from flask import Flask, render_template, request, redirect, url_for, flash
import shlex # コマンドインジェクションを防ぐために使用

app = Flask(__name__)
# セッションメッセージを有効にするための設定
app.secret_key = 'your_secret_key_here'

# 参照するベースディレクトリを設定（今回はプロジェクトのルートディレクトリ）
# 🚨 セキュリティのため、公開するディレクトリを限定してください
BASE_DIR = os.path.abspath(os.path.dirname(__file__))

@app.route('/')
@app.route('/browse')
def browse():
    """ディレクトリ内のファイルとディレクトリを一覧表示する"""
    try:
        items = os.listdir(BASE_DIR)
        
        # ディレクトリとファイルの区別をつける
        files = [item for item in items if os.path.isfile(os.path.join(BASE_DIR, item))]
        dirs = [item for item in items if os.path.isdir(os.path.join(BASE_DIR, item))]

        return render_template('browse.html', files=files, dirs=dirs, current_dir=BASE_DIR)

    except FileNotFoundError:
        flash("指定されたディレクトリが見つかりません。")
        return redirect(url_for('browse'))

@app.route('/handle_file_path', methods=['POST'])
def handle_file_path():
    """クライアントから送られたファイルパスをsubprocessに渡す"""
    file_name = request.form.get('file_name')
    if file_name:
        full_path = os.path.join(BASE_DIR, file_name)

        # 🚨 警告: ユーザー入力がそのままコマンドになるため、セキュリティリスクがあります。
        # 実際には、`shlex.split`や`subprocess.run`の`check=True`などを使い、
        # 厳密なバリデーションを行う必要があります。
        
        try:
            # 例として、選択されたファイルのパスを`echo`コマンドに渡す
            # 実際の用途に合わせてコマンドを置き換えてください
            command = f"echo 'ユーザーが選択したファイル: {full_path}'"
            
            # shlex.splitを使ってコマンドを安全に分割
            result = subprocess.run(shlex.split(command), capture_output=True, text=True, check=True)
            
            # ターミナルに出力を表示
            print(result.stdout)
            
            flash(f"ファイルパスが subprocess に渡されました: {full_path}")

        except subprocess.CalledProcessError as e:
            flash(f"コマンドの実行に失敗しました: {e}")
            print(f"Error: {e.stderr}")
            
    return redirect(url_for('browse'))

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')