

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
          this.markers.splice(0, i);
          map.removeLayer(this.markers[i].marker);
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
        if (this.isVisible) {
          map.removeLayer(this.markers[i].marker);
        }
      }
    }
    this.markers.splice(0, i);
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
    // if(!isRotationON){
    //   return;
    // }
    for (var i = 0; i < this.markers.length; i++) {
      map.removeLayer(this.markers[i].marker);
      this.markers[i].marker.setRotationAngle(this.markers[i].getRelativeRotationAngle());
      if (this.isVisible) { 
        this.markers[i].marker.addTo(map);
       }
    }
  }
}

// class crowdMarker extends AdvancedMarker{
//   constructor(...args){
//     super(args);
//   }
//   velocity;
//   angle;
// }

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
// let init_mark;
// let initMark;
// let second_mark;
// let secondMark;
// let gps_init_pub;
// let gps_second_pub;

let gps_markers = [];
let latest_gps_marker;
let odom_markers = [];
let latest_odom_marker;
let filtered_markers = [];
let latest_filtered_marker;
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
let maxMarkerTime = 600000;
let wayPointMarker;
let isGPSVisible;

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

  //アラートの宣言
  var approachingAlert = new AlertAdmin("alert approach");

  var Alert2 = new AlertAdmin("alert alert2");

  var Alert3 = new AlertAdmin("alert alert3");

  approachingAlert.setAlertLevel(0);

  Alert2.setAlertLevel(1);

  Alert3.setAlertLevel(2);

  map.addControl(new DragControl({ position: 'topright' }));

  center_latLng = map.getCenter();
  //document.getElementById("center").textContent = "中心座標：（ 緯度 " + center_latLng.lng + " , 経度 " + center_latLng.lat + " ）"

  // アイコンを地図に追加

  // var konzatuIcon = [createKonzatuIcon('red', 3), createKonzatuIcon('blue', 1), createKonzatuIcon('blue', 2)];
  // konzatu_control.addToLayer(new AdvancedMarker(new L.marker(center_latLng, { icon: konzatuIcon[0] }), 0,124));

  // konzatu_control.addToLayer(new AdvancedMarker(new L.marker([34.64890218832923, 135.3858304023743], { icon: konzatuIcon[1] }), 1,  23));
  // konzatu_control.addToLayer(new AdvancedMarker(new L.marker([34.64890218833923, 135.3858304013743], { icon: konzatuIcon[2] }), 2,  158 ));
  // konzatu_control.addToLayer(new AdvancedMarker(new L.marker([34.64890218833933, 135.3858304013753], { icon: konzatuIcon[1] }), 5,  8 ));
  // konzatu_control.addToLayer(new AdvancedMarker(new L.marker([34.64890218833953, 135.3858304013749], { icon: konzatuIcon[0] }), 6, 64 ));
  // init_posのマーカーアイコン作成.
  init_mark = L.divIcon({ // CSSを使ったDivIconを作成
    className: 'init_pose',
    bgPos: [18, 18]
  });

  // second_posのマーカーアイコン作成.
  second_mark = L.divIcon({ // CSSを使ったDivIconを作成
    className: 'second_pose',
    bgPos: [18, 18]
  });


  let crossMark = L.marker(map.getCenter(), { // マーカとして登録
    icon: cross, zIndexOffset: 100, interactive: false
  }).addTo(map);
  map.on('move', function () { // mousemoveイベントでマーカを移動
    center_latLng = map.getCenter();
    crossMark.setLatLng(center_latLng);
    console.log(center_latLng);
    //document.getElementById("center").textContent = "中心座標：（ 緯度 " + center_latLng.lng + " , 経度 " + center_latLng.lat + " ）";
  });


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
  autoMobile_start_pub = new ROSLIB.Topic({
    ros: ros,
    name: '/expo_start',
    messageType: 'std_msgs/string'
  });
  autoMobile_stop_pub = new ROSLIB.Topic({
    ros: ros,
    name: '/expo_stop',
    messageType: 'std_msgs/string'
  });
  wayPoint_pub = new ROSLIB.Topic({
    ros: ros,
    name: '/expo_waypoint',
    messageType: 'sensor_msgs/NavSatFix'
  });

  // // subscriberの設定.
  // let odom_sub = new ROSLIB.Topic({
  //   ros: ros,
  //   name: '/fix/utm',
  //   messageType: 'sensor_msgs/NavSatFix'
  // });
  // let filtered_sub = new ROSLIB.Topic({
  //   ros: ros,
  //   name: '/odometry/utm_filtered',
  //   messageType: 'nav_msgs/Odometry'
  // });
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
  // // callback関数.
  // odom_sub.subscribe(function (message) {
  //   console.log("subscribed odom!!!");

  //   //最新点だけ表示.
  //   if (latest_odom_marker != null) { map.removeLayer(latest_odom_marker); }
  //   latest_odom_marker = L.circleMarker([message.latitude, message.longitude], latestOdomIcon).addTo(map);
  //   map.setView([message.latitude, message.longitude]);

  //   //gpsと同じ間隔でodometryマーカー作成.
  //   if (mark_odom_flg) {
  //     odom_markers.push(L.marker([message.latitude, message.longitude], { icon: blueIcon }).addTo(map));
  //     document.getElementById("processing").textContent = "(緯度:経度) = (" + message.latitude + ":" + message.longitude + ")";
  //     console.log("(緯度:経度) = (" + message.latitude + ":" + message.longitude + ")");
  //     mark_odom_flg = false;
  //   }
  // });

  // filtered_sub.subscribe(function (message) {
  //   console.log("subscribed odom!!!");
  //   //msgからUTMゾーン情報を取得.
  //   const sub_frame_id = message.header.frame_id;
  //   const firstNum = sub_frame_id.indexOf('_') + 1;
  //   const lastNum = sub_frame_id.length - 1;
  //   let utm_zone = Number(sub_frame_id.substr(firstNum, (lastNum - firstNum)));
  //   console.log(utm_zone);

  //   //proj4jsにUTMゾーンとUTM座標を渡して緯度経度に変換.
  //   let message_xy = [message.pose.pose.position.x, message.pose.pose.position.y];
  //   let lon_lat = proj4(UTM_zone_to_epsg(utm_zone)).inverse(message_xy);

  //   //最新点だけ表示.
  //   if (latest_filtered_marker != null) { map.removeLayer(latest_filtered_marker); }
  //   latest_filtered_marker = L.circleMarker([lon_lat[1], lon_lat[0]], latestfilteredIcon).addTo(map);

  //   //gpsと同じ間隔でodometryマーカー作成.
  //   if (mark_filtered_flg) {
  //     filtered_markers.push(L.marker([lon_lat[1], lon_lat[0]], { icon: yellowIcon }).addTo(map));
  //     mark_filtered_flg = false;
  //   }
  // });


  odom_fix_sub.subscribe(function (message) {
    console.log("subscribed gps!!!");
    let tempxy;
    //最新点だけ表示.
    if (latest_gps_marker != null) {
      map.removeLayer(latest_gps_marker);
    }
    latest_gps_marker = L.marker([message.fix.latitude, message.fix.longitude], { icon: selfLocationIcon });

    // L.marker([message.fix.latitude, message.fix.longitude],{icon:redIcon}).addTo(map);
    //時間を過ぎたマーカーを削除
    stray_control.deleteTimeOverMarker();
    lost_prop_control.deleteTimeOverMarker();

    if (isRotationON) {
      map.panTo([message.fix.latitude, message.fix.longitude]);
      map_rotate_angle = orientationToAngle(message.orientation);
      map.setBearing(map_rotate_angle);
      konzatu_control.renewRotationMarker();
    }
    else {
      var sensorAngle = 2 * Math.acos(message.orientation.w) * (180 / Math.PI) - 90;
      if (sensorAngle < 0) {
        sensorAngle += 360;
      }
      latest_gps_marker.setRotationAngle(sensorAngle);
    }
    if (isGPSVisible) {
      latest_gps_marker.addTo(map);
    }
    mark_odom_flg = true;
    mark_filtered_flg = true;
    count_gps++;
  });
  //document.getElementById("processing").textContent = "緯度経度テストjs読み込み完";]
  generateTileGroup([34.64695159902189, 135.37847645406802], 0.00045, 0.00045, 30, 30);

  maigo_sub.subscribe(function (message) {
    //map.panTo([message.latitude, message.longitude]);
    stray_control.addToLayer(new AdvancedMarker(L.marker([message.latitude, message.longitude], { icon: strayIcon })));
  })

  otosimono_sub.subscribe(function (message) {
    //map.panTo([message.latitude, message.longitude]);
    lost_prop_control.addToLayer(new AdvancedMarker(L.marker([message.latitude, message.longitude], { icon: lostPropIcon })));
  })

  konzatu_sub.subscribe(function (message) {
    if (message.persons == null) {
      return;
    }
    var persons = message.persons;
    for (var i = 0; i < persons.length; i++) {
      var icon = createKonzatuIcon(persons[i].velocity + 1);
      var rotAngle = orientationToAngle(persons[i].orientation);
      konzatu_control.addToLayer(new AdvancedMarker(L.marker([persons[i].latitude, persons[i].longitude], { icon: icon }), persons[i].id, rotAngle));
    }
  })
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
  return 2 * Math.acos(orientation.w) * (180 / Math.PI) - 90;
}
// function pub_init_pose() {

//   // ===== 既存マーカーがある場合は削除====
//   if (initMark != null) {
//     map.removeLayer(initMark);
//     initMark = null;
//   }
//   // ====== mapマーカー作成======
//   initMark = L.marker(center_latLng, { // マーカ登録.
//     icon: init_mark, zIndexOffset: 100, interactive: false
//   }).addTo(map);

//   // ====== 標準偏差の取得======
//   const sigma_element = document.getElementById('sigma');
//   const sigma = sigma_element.value;

//   // ====== 高度情報の取得とpublish. ======
//   // ブラウザの入力から地上高を取得
//   const ground_height_element = document.getElementById('ground_height');
//   const ground_height = ground_height_element.value

//   // ====== proj4jsでUTM座標に変換 ======
//   const utm_zone = judge_UTM_zone(center_latLng.lng);
//   console.log("EPSG_code = " + UTM_zone_to_epsg(utm_zone));
//   const utm_xy = proj4(UTM_zone_to_epsg(utm_zone)).forward([center_latLng.lng, center_latLng.lat]);
//   if (center_latLng.lat < 0) { xy_y += 10000000.0; }

//   // ====== utm_odometry_nodeの形式に合わせたフレームIDの作成 ======
//   const utm_frame_id = `utm/utm_${utm_zone}Z`;

//   console.log(`frame_id = ${utm_frame_id}`);


//   // geolonia の高度取得APIを使用して、指定座標のジオイド高＋標高＋地上高を計算.
//   // https://blog.geolonia.com/2023/04/14/geoid-api.html
//   const Http = new XMLHttpRequest();
//   const url = `https://api-vt.geolonia.com/api/altitude?lat=${center_latLng.lat}&lng=${center_latLng.lng}`;
//   Http.open("GET", url);
//   Http.send();
//   Http.onreadystatechange = function () {
//     if (this.readyState == 4 && this.status == 200) {
//       console.log(Http.responseText);

//       //geoloniaから取得したJSON形式テキストをjavascriptオブジェクトに変換.
//       const alt_obj = JSON.parse(Http.responseText);

//       //ジオイド高(geoid),標高(altitude),地上高(ground_height)
//       const pub_altitude = Number(alt_obj.geoid) + Number(alt_obj.altitude) + Number(ground_height);
//       console.log(`publish altitude = ${pub_altitude}`);

//       // publishメッセージの作成.
//       let init_pose = new ROSLIB.Message({
//         header: {
//           frame_id: utm_frame_id
//         },
//         child_frame_id: '',
//         pose: {
//           // 姿勢情報
//           pose: {
//             position: {
//               x: utm_xy[0],
//               y: utm_xy[1],
//               z: pub_altitude
//             }
//           },
//           covariance: [sigma * sigma, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, sigma * sigma, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, sigma * sigma, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
//         }
//       });

//       // publish
//       gps_init_pub.publish(init_pose);

//     }
//   }
// }

// // second_pose
// //publish用関数
// function pub_second_pose() {

//   // ===== 既存マーカーがある場合は削除====
//   if (secondMark != null) {
//     map.removeLayer(secondMark);
//     secondMark = null;
//   }
//   // ====== mapマーカー作成======
//   secondMark = L.marker(center_latLng, { // マーカ登録.
//     icon: second_mark, zIndexOffset: 100, interactive: false
//   }).addTo(map);

//   // ====== 高度情報の取得とpublish. ======
//   // ブラウザの入力から地上高を取得
//   const ground_height_element = document.getElementById('ground_height');
//   const ground_height = ground_height_element.value

//   // ====== proj4jsでUTM座標に変換 ======
//   const utm_zone = judge_UTM_zone(center_latLng.lng);
//   console.log("EPSG_code = " + UTM_zone_to_epsg(utm_zone));
//   const utm_xy = proj4(UTM_zone_to_epsg(utm_zone)).forward([center_latLng.lng, center_latLng.lat]);
//   if (center_latLng.lat < 0) { xy_y += 10000000.0; }

//   // ====== utm_odometry_nodeの形式に合わせたフレームIDの作成 ======
//   const utm_frame_id = `utm/utm_${utm_zone}Z`;

//   // geolonia の高度取得APIを使用して、指定座標のジオイド高＋標高＋地上高を計算.
//   // https://blog.geolonia.com/2023/04/14/geoid-api.html
//   const Http = new XMLHttpRequest();
//   const url = `https://api-vt.geolonia.com/api/altitude?lat=${center_latLng.lat}&lng=${center_latLng.lng}`;
//   Http.open("GET", url);
//   Http.send();
//   Http.onreadystatechange = function () {
//     if (this.readyState == 4 && this.status == 200) {
//       console.log(Http.responseText);

//       //geoloniaから取得したJSON形式テキストをjavascriptオブジェクトに変換.
//       const alt_obj = JSON.parse(Http.responseText);

//       //ジオイド高(geoid),標高(altitude),地上高(ground_height)
//       const pub_altitude = Number(alt_obj.geoid) + Number(alt_obj.altitude) + Number(ground_height);
//       console.log(`publish altitude = ${pub_altitude}`);

//       // publishメッセージの作成.
//       let second_pose = new ROSLIB.Message({
//         header: {
//           frame_id: utm_frame_id
//         },
//         child_frame_id: '',
//         pose: {
//           // 姿勢情報
//           pose: {
//             position: {
//               x: utm_xy[0],
//               y: utm_xy[1],
//               z: pub_altitude
//             }
//           },
//           covariance: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
//         }
//       });
//       // publish
//       gps_second_pub.publish(second_pose);
//     }
//   }
// }
