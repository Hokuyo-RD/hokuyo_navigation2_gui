#!/usr/bin/env python3

from flask import Flask, request, jsonify, render_template
import subprocess

app = Flask(__name__)

@app.route('/indoor_run')
def indoor_run():
    return render_template('indoor_run.html')

@app.route('/outdoor_run')
def outdoor_run():
    return render_template('outdoor_run.html')

@app.route('/stop')
def stop_run():
    return render_template('stop.html')

@app.route('/tools')
def tools_run():
    return render_template('tools.html')

@app.route('/wizurg', methods=['GET', 'POST'])
def trigger_script():
    if request.method == 'GET':
        return render_template('index.html')
    
    elif request.method == 'POST':
        command = request.form.get("command") or request.get_json().get("command")
        if command == "indoor_run":
            subprocess.run(["/home/hokuyo/catkin_ws/src/expo_wizurg/scripts/expo_in.sh"])
            return render_template('indoor_run.html')
        elif command == "outdoor_run":
            subprocess.run(["/home/hokuyo/catkin_ws/src/expo_wizurg/scripts/expo_out.sh"])
            return render_template('outdoor_run.html')
        elif command == "stop":
            subprocess.run(["/home/hokuyo/catkin_ws/src/expo_wizurg/scripts/web_kill_all_rosnode.sh"])
            return render_template('stop.html')
        elif command == "tools":
            subprocess.run(["/home/hokuyo/catkin_ws/src/expo_wizurg/scripts/expo_start.sh"])
            return render_template('tools.html')
        elif command == "map":
            return render_template('map.html')
        else:
            print(command)
            return "Unknown command", 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5050)