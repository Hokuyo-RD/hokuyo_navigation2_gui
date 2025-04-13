#!/usr/bin/env python3

from flask import Flask, request, jsonify, render_template
from flask_sockets import Sockets
import asyncio
import websockets
import ssl  # 追記

app = Flask(__name__)
sockets = Sockets(app)

ROSBRIDGE_URI = "ws://localhost:9090"  # 内部の ROSBridge WebSocket サーバーの URI

async def forward(ws, target):
    try:
        while True:
            try:
                if isinstance(ws, websockets.legacy.client.WebSocketClientProtocol):
                    message = await ws.recv()  # websockets の受信
                    await target.send(message)
                elif isinstance(ws, WebSocket):  # gevent-websocket の受信
                    message = ws.receive()
                    if message is None:
                        print(f"Forwarder (gevent): Received None, connection closed: {ws}")
                        break
                    await target.send(message)
                else:
                    print(f"Forwarder: Unknown WebSocket type: {type(ws)}")
                    break
            except websockets.exceptions.ConnectionClosedOK:
                print(f"Forwarder: Connection closed gracefully: {ws}")
                break
            except websockets.exceptions.ConnectionClosedError as e:
                print(f"Forwarder: Connection closed unexpectedly: {ws}, error: {e}")
                break
            except Exception as e:
                print(f"Forwarder: Error during receive/send on {ws}: {e}, type: {e.__class__.__name__}")
                break
    finally:
        print(f"Forwarder: Exiting for {ws}")

async def proxy(websocket):
    ros_ws = None
    try:
        ros_ws = await websockets.connect(ROSBRIDGE_URI)
        print(f"Proxy: Connected to ROSBridge: {ROSBRIDGE_URI}")
        await asyncio.gather(
            forward(websocket, ros_ws),
            forward(ros_ws, websocket),
        )
    except ConnectionRefusedError:
        print(f"Proxy: Error: Could not connect to ROSBridge at {ROSBRIDGE_URI}. Make sure it's running.")
    except Exception as e:
        print(f"Proxy: WebSocket proxy error: {e}, type: {e.__class__.__name__}")
    finally:
        print("Proxy: WebSocket proxy connection closed.")
        if ros_ws and not ros_ws.closed:
            await ros_ws.close()

@sockets.route('/ws')
def websocket_handler(ws):
    asyncio.run(proxy(ws))

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
        elif command == "map":
            return render_template('map.html')
        else:
            print(command)
            return "Unknown command", 400

if __name__ == '__main__':
    from gevent import pywsgi
    from geventwebsocket.handler import WebSocketHandler
    from geventwebsocket.websocket import WebSocket  # Import WebSocket class
    import ssl  # 追記

    # HTTPS/WSS のためのコンテキスト (Tailscale Serve が証明書を管理している場合は不要な可能性あり)
    # context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
    # context.load_cert_chain('path/to/your/certificate.crt', 'path/to/your/private.key')

    server = pywsgi.WSGIServer(('0.0.0.0', 5050), app, handler_class=WebSocketHandler) # , ssl_context=context
    print("WebSocket Proxy server started at ws://0.0.0.0:5050/ws") # 外部からは wss:// で接続
    server.serve_forever()