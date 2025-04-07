from flask import Flask, request, jsonify
import subprocess

app = Flask(__name__)

@app.route('/trigger', methods=['GET', 'POST'])
def trigger_script():
    if request.method == 'GET':
        return '''
            <form action="/trigger" method="post">
                <input type="text" name="command" value="run_script">
                <input type="submit" value="Run Script">
            </form>
        '''
    elif request.method == 'POST':
        command = request.form.get("command") or request.get_json().get("command")
        if command == "run_script":
            subprocess.run(["./sample_script.sh"])
            return "Script executed"
        else:
            return "Unknown command", 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)

