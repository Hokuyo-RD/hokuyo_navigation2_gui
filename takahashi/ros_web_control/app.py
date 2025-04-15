from flask import Flask, render_template
from flask_sockets import Sockets
import asyncio
import websockets
import ssl  # 追記
from geventwebsocket.websocket import WebSocket  # Import WebSocket class

app = Flask(__name__)
sockets = Sockets(app)

ROSBRIDGE_URI = "ws://localhost:9090"  # ROSBridge の URI (ローカルホスト)

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
        await asyncio.gather(
            forward(websocket, ros_ws),
            forward(ros_ws, websocket),
        )
    except ConnectionRefusedError:
        print(f"Error: Could not connect to ROSBridge at {ROSBRIDGE_URI}. Make sure it's running.")
    except Exception as e:
        print(f"WebSocket proxy error: {e}, type: {e.__class__.__name__}")
    finally:
        print("WebSocket proxy connection closed.")
        if ros_ws and not ros_ws.closed:
            await ros_ws.close()

@sockets.route('/ws')
def websocket_handler(ws):
    asyncio.run(proxy(ws))

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == '__main__':
    from gevent import pywsgi
    from geventwebsocket.handler import WebSocketHandler
    from geventwebsocket.websocket import WebSocket  # Import WebSocket class
    server = pywsgi.WSGIServer(('0.0.0.0', 5050), app, handler_class=WebSocketHandler)
    print("WebSocket Proxy server started at ws://0.0.0.0:5050/ws")
    server.serve_forever()