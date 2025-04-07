import asyncio
from bleak import BleakClient

# BLEデバイス設定
BLE_DEVICE_ADDRESS = "C8:47:80:18:B3:78"  # BLEデバイスのMACアドレス
BLE_SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb"  # サービスUUID
BLE_TX_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"  # 書き込み（送信）UUID
BLE_RX_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"  # 読み取り（受信）UUID

# 送信データ（16進数） 0x000004011355AA17
SEND_DATA = bytes.fromhex("000004011355AA17")

async def notification_handler(sender, data):
    """BLEデバイスからのデータを受信して表示"""
    print(f"受信データ: {data.hex().upper()}")
    
    # バイナリデータに変換
    #payload = bytes.fromhex(data)
    
    # インデックス12から4バイトのデータをリトルエンディアンで取得し、
    # 1000で割る (電圧値)
    voltage = int.from_bytes(data[12:16], byteorder='little') / 1000.0
    print(f"電圧値: {voltage:.3f}")

    # 電池残量
    remianingAh = int.from_bytes(data[62:64], byteorder='little') / 100.0
    capacityAh  = int.from_bytes(data[64:66], byteorder='little') / 100.0
    chargePercent = (remianingAh / capacityAh) * 100
    print(f"電池残量: {chargePercent:.3f}%")

async def main():
    async with BleakClient(BLE_DEVICE_ADDRESS) as client:
        print(f"{BLE_DEVICE_ADDRESS} に接続成功！")

        # 受信データの通知を開始
        await client.start_notify(BLE_RX_UUID, notification_handler)

        # データを送信
        print(f"送信データ: {SEND_DATA.hex().upper()}")
        await client.write_gatt_char(BLE_TX_UUID, SEND_DATA, response=True)

        # 5秒間データ受信を待つ
        await asyncio.sleep(5)

        # 受信データの通知を停止
        await client.stop_notify(BLE_RX_UUID)

if __name__ == "__main__":
    asyncio.run(main())
