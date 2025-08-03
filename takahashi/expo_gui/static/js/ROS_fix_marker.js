//混雑度のマーカーが透過されるまでのカウント
const maxMarkerRenewCount = 3;
//混雑度マーカーの透過度
const crowdOpacity = 0.5;

///アイコンを一括で管理するためのクラス
class LayerControlAdmin {
  constructor(layerName, isVisible, markers) {
    this.layerName = layerName;
    this.isVisible = isVisible;
    this.markers = markers;
    this.count = 3;
  }
  markers = [];

  isVisible = true;

  layerName = "";


  setAllOpacity = function (opacity) {
    for (var i = 0;i < this.markers.length;i++) {
      this.markers[i].renewCount += 1;
      if(this.markers[i].renewCount > maxMarkerRenewCount){
      const currentOpacity = this.markers[i].opacity == undefined ? 1.0 : this.markers[i].opacity;
      this.markers[i].setOpacity(currentOpacity * opacity);
      }
     }
  }

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
          this.markers.splice(i, 1);
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
    if (!isRotationON) {
      return;
    }
    for (var i = 0; i < this.markers.length; i++) {
      this.markers[i].marker.setRotationAngle(this.markers[i].getRelativeRotationAngle());
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
    this.renewCount = 0;
  }
  marker;
  renewCount;
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

  setOpacity = function (opacity) {
    this.marker.setOpacity(opacity);
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
let map_rotate_angle = 0;
const earth_radius = 6387137;

let gps_control = new LayerControlAdmin("自己位置", true, []);
let stray_control = new LayerControlAdmin("stray", true, []);
let lost_prop_control = new LostPropAdmin();
let konzatu_control = new LayerControlAdmin("konzatu", true, []);
let isRotationON = false;
let maxMarkerTime = 10000;
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

  L.control.scale().addTo(map);

  isGPSVisible = true;

  let sensorStateTable = new SensorStateAdmin();

  document.getElementById("sensorStateButton").onclick = function () { sensorStateTable.show() };

  document.getElementById("markerDuration").addEventListener("change", changeMaxMarkerTime);

  document.getElementById("h_QR").addEventListener("click", showQRDialog);

  maxMarkerTime = document.getElementById("markerDuration").value * 1000;

  console.log(maxMarkerTime);

  //アラートの宣言
  var approachingAlarm = new AlarmAdmin("alarm approach", "ロボットが障害物を検知し、停止しました。<br>スタート位置まで戻り、再起動を行ってください。");

  var odomNotSentAlarm = new AlarmAdmin("alarm odomNotSent", "自己位置の推定が停止しました。<br>スタート位置まで戻り、再起動を行ってください。");

  var sensorLostAlarm = new AlarmAdmin("alarm sensorLost", "自律走行に必要なセンサとの通信が途切れたため、ロボットが停止しました。<br>スタート位置まで戻り、再起動を行ってください。");

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
    //url: 'ws://localhost:9090'
    url: 'ws://100.108.154.115:9090'
    //url:'ws://0.0.0.0:9090'
    //url: 'ws://' + location.hostname + ':9090'
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



  let otosimono_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/lost_fix',
    messageType: 'expo_crowd_msgs/LostsFix'
  });

  let konzatu_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/crowd_fix',
    messageType: 'expo_crowd_msgs/CrowdFixEX'
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

  let sensors_status_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/sensors_state',
    messageType: 'expo_safety_manage/SensorState'
  })

  let estimated_pose_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/estimated_pose',
    messageType: 'geometry_msgs/PoseStamped'
  })

  odom_fix_sub.subscribe(function (message) {
    //最新点だけ表示.
    if (latest_gps_marker != null) {
      map.removeLayer(latest_gps_marker);
    }
    latest_gps_marker = L.marker([message.fix.latitude, message.fix.longitude], { icon: selfLocationIcon });
    lost_prop_control.showPopup([message.fix.latitude, message.fix.longitude]);

    // L.marker([message.fix.latitude, message.fix.longitude],{icon:redIcon}).addTo(map);
    //時間を過ぎたマーカーを削除
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

  otosimono_sub.subscribe(function (message) {
    if (message == null) {
      return;
    }
    for(var item of message.lostitem){

      lost_prop_control.addMarker(item.type,new AdvancedMarker(L.marker([item.latitude,item.longitude],{icon: getLostIconFromId(item.type) }), item.id));
    }
  })

  konzatu_sub.subscribe(function (message) {
    //var state;
    if (message == null) {
      return;
    }
    if (message.persons == null) {
      return;
    }
    /*
    if(message.persons.state == null){
      state = message.state;
    }
    */
    konzatu_control.deleteTimeOverMarker();
    konzatu_control.setAllOpacity(crowdOpacity);
    var persons = message.persons;
    for (var i = 0; i < persons.length; i++) {
      var icon = createKonzatuIcon(persons[i].velocity + 1, persons[i].state);
      var rotAngle = orientationToAngle(persons[i].orientation);
      konzatu_control.addToLayer(new AdvancedMarker(L.marker([persons[i].latitude, persons[i].longitude], { icon: icon }), persons[i].id, rotAngle));
    }
  })

  approach_alarm_sub.subscribe(function (message) {
    if (message == null) {
      return;
    }
    approachingAlarm.setAlarmLevel(message.data);
  })

  sensor_lost_alarm_sub.subscribe(function (message) {
    if (message == null) {
      return;
    }
    sensorLostAlarm.setAlarmLevel(message.data);
  })

  sensors_status_sub.subscribe(function (message) {
    if (message == null) {
      return;
    }
    sensorStateTable.setSensorState({
      UAM1: message.uam1,
      UAM2: message.uam2,
      UST1: message.ust1,
      UST2: message.ust2,
      YVT: message.yvt,
      YLM: message.ylm,
      GNSS: message.gnss
    });
  })

  estimated_pose_sub.subscribe(function (message) {
    if (odomTimer != null) {
      clearInterval(odomTimer);
      odomNotSentAlarm.setAlarmLevel(AlarmLevel.LOW);
    }
    odomTimer = odomNotSentAlarm.setTimer(AlarmLevel.HIGH);
  })


  lost_prop_control.showPopup([34.64878303599981,135.38642048835757]);
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
  var ret = 2 * Math.atan2(orientation.z, orientation.w) * (180 / Math.PI) - 90;
  if (ret < 0) {
    ret += 360;
  }
  return ret;
}
function changeMaxMarkerTime(e) {
  maxMarkerTime = e.target.value * 1000;
  console.log(maxMarkerTime);
}

function showQRDialog(e) {
  var path = getFilePath('/static/nakanoshima_QR.png');
  console.log(path);
  swal.fire({
    title: '中之島チャレンジ<br>ホームページ',
    imageUrl: path,
    imageWidth: 400,
    imageHeight: 400,
    imageAlt: 'Custom image',
  })
}

function getFilePath(filename) {
  return location.href.replace(location.pathname, "") + filename;
}
