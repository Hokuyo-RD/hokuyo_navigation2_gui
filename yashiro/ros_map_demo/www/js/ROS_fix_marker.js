

///アイコンを一括で管理するためのクラス
class LayerControlAdmin {
  constructor(layerName, isVisible, markers) {
    this.layerName = layerName;
    this.isVisible = isVisible;
    this.markers = markers;
  }
  markers = [];

  isVisible = true;

  layerName = "";


  addToLayer = function (marker) {
    this.deleteSameIDMarker(marker);
    if (this.isVisible) {
      this.markers.push(marker);
      marker.addTo(map);
    }
    else {
      this.markers.push(marker);
    }
  }

  visibleChanged = function (checked) {
    this.isVisible = checked;
    if (this.isVisible) {
      for (var i = 0; i < this.markers.length; i++) {
        this.markers[i].addTo(map);
      }
    }
    else {
      for (var i = 0; i < this.markers.length; i++) {
        if (this.markers[i].marker != null) {
          map.removeLayer(this.markers[i].marker);
        }
        else {
          map.removeLayer(this.markers[i]);
        }
      }
    }
  }
  ///IDが同じマーカーを削除
  deleteSameIDMarker = function (newMarker) {
    if (newMarker.id == null) {
      return;
    }
    for (var i = 0; i < this.markers.length; i++) {
      if (this.markers[i].id != null) {
        if (this.markers[i].id == newMarker.id) {
          map.removeLayer(this.markers[i].marker);
          this.markers.splice(i,1);
        }
      }
    }
  }
  ///マーカーの表示最大時間を過ぎたマーカーを削除する
  deleteTimeOverMarker = function () {
    var now = new Date();
    var nowTime = now.getTime();
    for (var i = 0; i < this.markers.length; i++) {
      var elapsedTime = this.markers[i].getElapsedTime(nowTime);
      ///マーカーの経過時間が単調減少することを前提とする。
      if (elapsedTime < maxMarkerTime) {
        this.markers.splice(0, i);
        return;
      }
      else {
          map.removeLayer(this.markers[i].marker);
      }
    }
    this.markers.splice(0, this.markers.length);
    return;
  }
  deleteAllMarker = function () {
    for (var i = 0; i < this.markers.length; i++) {
      {
        if (this.markers[i].marker != null) {
          map.removeLayer(this.markers[i].marker);
        }
        else {
          map.removeLayer(this.markers[i]);
        }
      }
    }
    this.markers = [];
  }
  ///回転しているマーカーの回転角度をマップに合わせて更新する。
  renewRotationMarker() {
     if(!isRotationON){
       return;
     }
    for (var i = 0; i < this.markers.length; i++) {
      map.removeLayer(this.markers[i].marker);
      this.markers[i].marker.setRotationAngle(this.markers[i].getRelativeRotationAngle());
      if (this.isVisible) {
        this.markers[i].marker.addTo(map);
      }
    }
  }
}


///マーカーの追加機能を実装するためのクラス
class AdvancedMarker {
  constructor(marker, id = null, rotationAngle = 0) {
    this.marker = marker;
    var now = new Date();
    this.startTime = now.getTime();
    this.rotationAngle = rotationAngle;
    this.id = id;
    this.marker.setRotationAngle(this.rotationAngle);
  }
  marker;
  //マーカーを置いた時刻
  startTime;
  //マーカーに対するid(int)
  id;
  //マーカーの回転角度
  rotationAngle;
  //マーカーを置いたときからの経過時間を計算
  getElapsedTime = function (nowTime) {
    var ret = nowTime - this.startTime;
    if (ret >= 0) {
      return ret;
    }
    else {
      return 0;
    }
  }
  addTo = function (map) {
    this.marker.setRotationAngle(this.rotationAngle);
    return this.marker.addTo(map);
  }

  //マップの回転角度に対するマーカーの回転角度を計算する。
  getRelativeRotationAngle() {
    var ret = this.rotationAngle + map_rotate_angle;
    if (ret > 360) {
      ret -= 360;
    }
    else if (ret < 0) {
      ret += 360;
    }
    return ret;
  }
}

function rotationSwitch(checked) {
  isRotationON = checked;
  if (!isRotationON) {
    map.setBearing(0);
    map_rotate_angle = 0;
    konzatu_control.renewRotationMarker();
  }
  else {
    map.setBearing(map_rotate_angle);
    konzatu_control.renewRotationMarker();
  }
}

let center_latLng;
let gps_markers = [];
let latest_gps_marker;
const length_per_side = [0.000045, 0.0000625];
let start_point = [34.64695159902189, 135.37847645406802];
const number_of_rows = 10;
const number_of_lines = 10;
let crowding_polygones = [];
let map_rotate_angle;
const earth_radius = 6387137;

let gps_control = new LayerControlAdmin("自己位置", true, []);
let stray_control = new LayerControlAdmin("stray", true, []);
let lost_prop_control = new LayerControlAdmin("lost_property", true, []);
let konzatu_control = new LayerControlAdmin("konzatu", true, []);
let isRotationON = true;
let maxMarkerTime = 300000;
let wayPointMarker;
let isGPSVisible;
let odomTimer = null;

function changeGPSMarkerVisible(checked) {
  isGPSVisible = checked;
  if (!latest_gps_marker) {
    return;
  }
  if (!isGPSVisible) {
    map.removeLayer(latest_gps_marker);
  }
  else {
    latest_gps_marker.addTo(map);
  }
}
function delete_all_markers() {
}

function pub_init_pose() { }
window.onload = (event) => {

  isGPSVisible = true;

  document.getElementById("markerDuration").addEventListener("change",changeMaxMarkerTime);

  document.getElementById("h_QR").addEventListener("click",showQRDialog)

  maxMarkerTime = document.getElementById("markerDuration").value*1000;

  console.log(maxMarkerTime);

  //アラートの宣言
  var approachingAlarm = new AlarmAdmin("alarm approach","ロボットが障害物を検知し、停止しました。<br>スタート位置まで戻り、再起動を行ってください。");

  var odomNotSentAlarm = new AlarmAdmin("alarm odomNotSent","自己位置の推定が停止しました。<br>スタート位置まで戻り、再起動を行ってください。");

  var sensorLostAlarm = new AlarmAdmin("alarm sensorLost","自立走行に必要なセンサとの通信が途切れたため、ロボットが停止しました。<br>スタート位置まで戻り、再起動を行ってください。");

  map.addControl(new DragControl({ position: 'topright' }));

  center_latLng = map.getCenter();


  // ROSとの接続.
  let count_gps = 0;
  let per_ = 5;
  let mark_odom_flg = false;
  let mark_filtered_flg = false;
  let ros = new ROSLIB.Ros({
    //url : 'ws://192.168.137.31:9090'
    //url : 'ws://192.168.0.157:9090'
    //url : 'ws://localhost:9090'
    url: 'ws://' + location.hostname + ':9090'
  });

  ros.on('connection', function () {
    console.log('Connected to websocket server.');
  });

  ros.on('error', function (error) {
    console.log('Error connecting to websocket server: ', error);
  });

  ros.on('close', function () {
    console.log('Connection to websocket server closed.');
  });

  // publisherの設定.
  gps_init_pub = new ROSLIB.Topic({
    ros: ros,
    name: '/odometry/gps_init',
    messageType: 'nav_msgs/Odometry'
  });
  gps_second_pub = new ROSLIB.Topic({
    ros: ros,
    name: '/odometry/gps_second',
    messageType: 'nav_msgs/Odometry'
  });

  wayPoint_pub = new ROSLIB.Topic({
    ros: ros,
    name: '/expo_waypoint',
    messageType: 'sensor_msgs/NavSatFix'
  });

  let odom_fix_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/odom_fix',
    messageType: 'expo_fix_msgs/FixWithOrientation'
  });

  let maigo_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/maigo_fix',
    messageType: 'sensor_msgs/NavSatFix'
  });

  let otosimono_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/otosimono_fix',
    messageType: 'sensor_msgs/NavSatFix'
  });

  let konzatu_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/crowd_fix',
    messageType: 'expo_crowd_msgs/CrowdFix'
  })

  let approach_alarm_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/approach_alarm',
    messageType: 'std_msgs/Int32'
  })
  let sensor_lost_alarm_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/sensor_lost_alarm',
    messageType: 'std_msgs/Int32'
  })



  odom_fix_sub.subscribe(function (message) {
    if (odomTimer != null) {
      clearInterval(odomTimer);
      odomNotSentAlarm.setAlarmLevel(AlarmLevel.LOW);
    }
    odomTimer = setTimer(AlarmLevel.HIGH);
    //最新点だけ表示.
    if (latest_gps_marker != null) {
      map.removeLayer(latest_gps_marker);
    }
    latest_gps_marker = L.marker([message.fix.latitude, message.fix.longitude], { icon: selfLocationIcon });

    // L.marker([message.fix.latitude, message.fix.longitude],{icon:redIcon}).addTo(map);
    //時間を過ぎたマーカーを削除
    stray_control.deleteTimeOverMarker();
    lost_prop_control.deleteTimeOverMarker();
    konzatu_control.deleteTimeOverMarker();

    if (isRotationON) {
      map.panTo([message.fix.latitude, message.fix.longitude]);
      map_rotate_angle = orientationToAngle(message.orientation);
      map.setBearing(map_rotate_angle);
      konzatu_control.renewRotationMarker();
    }
    else {
      var sensorAngle = 360 - orientationToAngle(message.orientation);
      latest_gps_marker.setRotationAngle(sensorAngle);
    }
    if (isGPSVisible) {
      latest_gps_marker.addTo(map);
    }
  });

  maigo_sub.subscribe(function (message) {
    stray_control.addToLayer(new AdvancedMarker(L.marker([message.latitude, message.longitude], { icon: strayIcon })));
  })

  otosimono_sub.subscribe(function (message) {
    lost_prop_control.addToLayer(new AdvancedMarker(L.marker([message.latitude, message.longitude], { icon: lostPropIcon })));
  })

  konzatu_sub.subscribe(function (message) {
    if (message.persons == null) {
      return;
    }
    konzatu_control.deleteTimeOverMarker();
    var persons = message.persons;
    for (var i = 0; i < persons.length; i++) {
      var icon = createKonzatuIcon(persons[i].velocity + 1);
      var rotAngle = orientationToAngle(persons[i].orientation);
      konzatu_control.addToLayer(new AdvancedMarker(L.marker([persons[i].latitude, persons[i].longitude], { icon: icon }), persons[i].id, rotAngle));
    }
  })

  approach_alarm_sub.subscribe(function (message) {
    approachingAlarm.setAlarmLevel(message.data);
  })

  sensor_lost_alarm_sub.subscribe(function (message) {
    sensorLostAlarm.setAlarmLevel(message.data);
  })

  odomTimer = odomNotSentAlarm.setTimer(AlarmLevel.HIGH);

};




// publish用関数(HTMLから呼び出すためにwindow.onload外で定義)
function pub_expo_start() {
  var message = new ROSLIB.Message({
    data: 'start'
  })
  autoMobile_start_pub.publish(message);
}

function pub_expo_stop() {
  var message = new ROSLIB.Message({
    data: 'stop'
  })
  autoMobile_start_pub.publish(message);
}

function pub_expo_wayPoint() {
  if (wayPointMarker) {
    var message = new ROSLIB.Message({
      latitude: wayPointMarker.getLatLng().lat,
      longitude: wayPointMarker.getLatLng().lng
    });
  }
  wayPoint_pub.publish(message);
}

//クオータニオンからマップ基準の角度（時計回り）に変換
function orientationToAngle(orientation) {
  var ret = 2 * Math.acos(orientation.w) * (180 / Math.PI) - 90;
  if (ret < 0) {
    ret += 360;
  }
  return ret;
}
function changeMaxMarkerTime(e){
  maxMarkerTime = e.target.value*1000;
  console.log(maxMarkerTime);
}

function showQRDialog(e){
  swal.fire({title: '中之島チャレンジ<br>ホームページ',
    imageUrl: 'Image/nakanoshima_QR.png',
    imageWidth: 400,
    imageHeight: 400,
    imageAlt: 'Custom image',})
}