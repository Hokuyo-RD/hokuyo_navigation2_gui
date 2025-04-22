
///アイコンを一括で管理するためのクラス
class IconControlAdmin {
  constructor(layerName, isVisible, markers) {
    this.layerName = layerName;
    this.isVisible = isVisible;
    this.markers = markers;
  }
  markers = [];

  isVisible = true;

  layerName = "";

  addToLayer = function (marker) {
    if (this.isVisible) {
      this.markers.push(marker.addTo(map));
    }
    else {
      this.markers.push(marker);
    }
  }
  visibleChanged = function (checked) {
    this.isVisible = checked;
    if (this.isVisible) {
      for (var i = 0;i< this.markers.length ;i++) {
        this.markers[i].addTo(map);
      }
    }
    else {
      for (var i = 0;i< this.markers.length ;i++) {
        map.removeLayer(this.markers[i]);
      }
    }
  }
}

function rotationSwitch(){
  if(isRotationON){
    map.setBearing (0);
    isRotationON = false;
  }
  else{
    isRotationON = true;
    map.setBearing(map_rotate_angle);
  }
}

let center_latLng;
let init_mark;
let initMark;
let second_mark;
let secondMark;
let gps_init_pub;
let gps_second_pub;

let gps_markers = [];
let latest_gps_marker;
let odom_markers = [];
let latest_odom_marker;
let filtered_markers = [];
let latest_filtered_marker;
const length_per_side = [0.000045, 0.0000625];
let start_point = [34.636875765664946, 135.41315674781802];
let polygon_points = [start_point,
  [start_point[0] + length_per_side[0], start_point[1]],
  [start_point[0] + length_per_side[0], start_point[1] + length_per_side[1]],
  [start_point[0], start_point[1] + length_per_side[1]]];
const number_of_rows = 10;
const number_of_lines = 10;
let crowding_polygones = [];
let map_rotate_angle ;
let previos_message;
const earth_radius = 6387137;

let gps_control = new IconControlAdmin("自己位置", true, []);
let stray_control = new IconControlAdmin("stray", true, []);
let lost_prop_control = new IconControlAdmin("lost_property", true, []);
let isRotationON = true;

function delete_all_markers() {

  for (let i = 0; i < gps_markers.length; i++) {
    map.removeLayer(gps_markers[i]);
  }
  gps_markers = [];

  for (let i = 0; i < odom_markers.length; i++) {
    map.removeLayer(odom_markers[i]);
  }
  odom_markers = [];

  for (let i = 0; i < filtered_markers.length; i++) {
    map.removeLayer(filtered_markers[i]);
  }
  filtered_markers = [];

  map.removeLayer(latest_gps_marker);
  latest_gps_marker = null;

  map.removeLayer(latest_odom_marker);
  latest_odom_marker = null;

  map.removeLayer(latest_filtered_marker);
  latest_filtered_marker = null;
}

function pub_init_pose() { }
window.onload = (event) => {

  center_latLng = map.getCenter();
  document.getElementById("center").textContent = "中心座標：（ 緯度 " + center_latLng.lng + " , 経度 " + center_latLng.lat + " ）"


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
    document.getElementById("center").textContent = "中心座標：（ 緯度 " + center_latLng.lng + " , 経度 " + center_latLng.lat + " ）";
  });
  ///polygon追加
  crowding_polygones.push(L.polygon(polygon_points, { color: "#" + Math.floor(Math.random() * 16777215).toString(16), weight: 5, fill: true, fillColor: "#ff8000", opacity: Math.random() }).addTo(map));

  for (var i = 1; i < number_of_rows; i++) {
    for (var j = 1; j < number_of_lines; j++) {
      polygon_points = [polygon_points[3],
      [polygon_points[3][0] + length_per_side[0], polygon_points[3][1]],
      [polygon_points[3][0] + length_per_side[0], polygon_points[3][1] + length_per_side[1]],
      [polygon_points[3][0], polygon_points[3][1] + length_per_side[1]]];
      crowding_polygones.push(L.polygon(polygon_points, { color: "#" + Math.floor(Math.random() * 16777215).toString(16), weight: 5, fill: true, fillColor: "#" + Math.floor(Math.random() * 16777215).toString(16), opacity: Math.random() }).addTo(map));
    }
    polygon_points[3] = [start_point[0] + length_per_side[0] * i, start_point[1]];
  };

  crowd_control = new IconControlAdmin("crowd",true,crowding_polygones);

  // ROSとの接続.
  let count_gps = 0;
  let per_ = 5;
  let mark_odom_flg = false;
  let mark_filtered_flg = false;
  let ros = new ROSLIB.Ros({
    //url : 'ws://192.168.137.31:9090'
    //url : 'ws://192.168.0.157:9090'
    url : 'ws://100.108.154.115:9090'
    //url: 'ws://' + location.hostname + ':9090'
    //url: 'ws://192.168.0.8:9090'
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


  // subscriberの設定.
  let odom_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/fix/utm',
    messageType: 'sensor_msgs/NavSatFix'
  });
  let filtered_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/odometry/utm_filtered',
    messageType: 'nav_msgs/Odometry'
  });
  let gps_sub = new ROSLIB.Topic({
    ros: ros,
    name: '/fix',
    messageType: 'sensor_msgs/NavSatFix'
  });

  // callback関数.
  odom_sub.subscribe(function (message) {
    console.log("subscribed odom!!!");

    //最新点だけ表示.
    if (latest_odom_marker != null) { map.removeLayer(latest_odom_marker); }
    latest_odom_marker = L.circleMarker([message.latitude, message.longitude], latestOdomIcon).addTo(map);
    map.setView([message.latitude, message.longitude]);

    //gpsと同じ間隔でodometryマーカー作成.
    if (mark_odom_flg) {
      odom_markers.push(L.marker([message.latitude, message.longitude], { icon: blueIcon }).addTo(map));
      document.getElementById("processing").textContent = "(緯度:経度) = (" + message.latitude + ":" + message.longitude + ")";
      console.log("(緯度:経度) = (" + message.latitude + ":" + message.longitude + ")");
      mark_odom_flg = false;
    }
  });

  filtered_sub.subscribe(function (message) {
    console.log("subscribed odom!!!");
    //msgからUTMゾーン情報を取得.
    const sub_frame_id = message.header.frame_id;
    const firstNum = sub_frame_id.indexOf('_') + 1;
    const lastNum = sub_frame_id.length - 1;
    let utm_zone = Number(sub_frame_id.substr(firstNum, (lastNum - firstNum)));
    console.log(utm_zone);

    //proj4jsにUTMゾーンとUTM座標を渡して緯度経度に変換.
    let message_xy = [message.pose.pose.position.x, message.pose.pose.position.y];
    let lon_lat = proj4(UTM_zone_to_epsg(utm_zone)).inverse(message_xy);

    //最新点だけ表示.
    if (latest_filtered_marker != null) { map.removeLayer(latest_filtered_marker); }
    latest_filtered_marker = L.circleMarker([lon_lat[1], lon_lat[0]], latestfilteredIcon).addTo(map);

    //gpsと同じ間隔でodometryマーカー作成.
    if (mark_filtered_flg) {
      filtered_markers.push(L.marker([lon_lat[1], lon_lat[0]], { icon: yellowIcon }).addTo(map));
      mark_filtered_flg = false;
    }
  });


  gps_sub.subscribe(function (message) {
    console.log("subscribed gps!!!");
    let tempxy;
    //最新点だけ表示.
    if (latest_gps_marker != null) { 
            map.removeLayer(latest_gps_marker); }
    latest_gps_marker = L.marker([message.latitude, message.longitude], { icon: redIcon }).addTo(map);
    if(previos_message != null && isRotationON){
      map.panTo([message.latitude, message.longitude]);      
      var latest_latlng ={longitude:message.longitude*(180/Math.PI),latitude:message.latitude*(180/Math.PI)};
      var previos_latlng ={longitude:previos_message.longitude*(180/Math.PI),latitude:previos_message.latitude*(180/Math.PI)};
      ///最新点と１つ前の点からセンサの進行方向を計算
      var dx = earth_radius*(latest_latlng.longitude - previos_latlng.longitude)*Math.cos((previos_latlng.latitude + latest_latlng.latitude)/2);
      var dy = earth_radius*(latest_latlng.latitude - previos_latlng.latitude);
      map_rotate_angle = -Math.atan2(dx,dy)* (180 / Math.PI);
      //map_rotate_angle = Math.atan2(message.latitude - previos_message.latitude,message.longitude - previos_message.longitude)* (180 / Math.PI);
      map.setBearing(map_rotate_angle);
    }
    previos_message = message;
    //gpsのマーカー作成して、odometry側フラグの操作.
    if (count_gps % per_ == 0) {
      var random = Math.random()*100;
      //100 * Math.random();
      if (random < 50) {
        stray_control.addToLayer(L.marker([message.latitude, message.longitude], { icon: strayIcon }));
      }
      else {
        lost_prop_control.addToLayer(L.marker([message.latitude, message.longitude],{icon: lostPropIcon}));
      }
      mark_odom_flg = true;
      mark_filtered_flg = true;
    }
    count_gps++;
  });
  //document.getElementById("processing").textContent = "緯度経度テストjs読み込み完";
};



// publish用関数(HTMLから呼び出すためにwindow.onload外で定義)
function pub_init_pose() {

  // ===== 既存マーカーがある場合は削除====
  if (initMark != null) {
    map.removeLayer(initMark);
    initMark = null;
  }
  // ====== mapマーカー作成======
  initMark = L.marker(center_latLng, { // マーカ登録.
    icon: init_mark, zIndexOffset: 100, interactive: false
  }).addTo(map);

  // ====== 標準偏差の取得======
  const sigma_element = document.getElementById('sigma');
  const sigma = sigma_element.value;

  // ====== 高度情報の取得とpublish. ======
  // ブラウザの入力から地上高を取得
  const ground_height_element = document.getElementById('ground_height');
  const ground_height = ground_height_element.value

  // ====== proj4jsでUTM座標に変換 ======
  const utm_zone = judge_UTM_zone(center_latLng.lng);
  console.log("EPSG_code = " + UTM_zone_to_epsg(utm_zone));
  const utm_xy = proj4(UTM_zone_to_epsg(utm_zone)).forward([center_latLng.lng, center_latLng.lat]);
  if (center_latLng.lat < 0) { xy_y += 10000000.0; }

  // ====== utm_odometry_nodeの形式に合わせたフレームIDの作成 ======
  const utm_frame_id = `utm/utm_${utm_zone}Z`;

  console.log(`frame_id = ${utm_frame_id}`);


  // geolonia の高度取得APIを使用して、指定座標のジオイド高＋標高＋地上高を計算.
  // https://blog.geolonia.com/2023/04/14/geoid-api.html
  const Http = new XMLHttpRequest();
  const url = `https://api-vt.geolonia.com/api/altitude?lat=${center_latLng.lat}&lng=${center_latLng.lng}`;
  Http.open("GET", url);
  Http.send();
  Http.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      console.log(Http.responseText);

      //geoloniaから取得したJSON形式テキストをjavascriptオブジェクトに変換.
      const alt_obj = JSON.parse(Http.responseText);

      //ジオイド高(geoid),標高(altitude),地上高(ground_height)
      const pub_altitude = Number(alt_obj.geoid) + Number(alt_obj.altitude) + Number(ground_height);
      console.log(`publish altitude = ${pub_altitude}`);

      // publishメッセージの作成.
      let init_pose = new ROSLIB.Message({
        header: {
          frame_id: utm_frame_id
        },
        child_frame_id: '',
        pose: {
          // 姿勢情報
          pose: {
            position: {
              x: utm_xy[0],
              y: utm_xy[1],
              z: pub_altitude
            }
          },
          covariance: [sigma * sigma, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, sigma * sigma, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, sigma * sigma, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
        }
      });

      // publish
      gps_init_pub.publish(init_pose);

    }
  }
}

// second_pose 
//publish用関数
function pub_second_pose() {

  // ===== 既存マーカーがある場合は削除====
  if (secondMark != null) {
    map.removeLayer(secondMark);
    secondMark = null;
  }
  // ====== mapマーカー作成======
  secondMark = L.marker(center_latLng, { // マーカ登録.
    icon: second_mark, zIndexOffset: 100, interactive: false
  }).addTo(map);

  // ====== 高度情報の取得とpublish. ======
  // ブラウザの入力から地上高を取得
  const ground_height_element = document.getElementById('ground_height');
  const ground_height = ground_height_element.value

  // ====== proj4jsでUTM座標に変換 ======
  const utm_zone = judge_UTM_zone(center_latLng.lng);
  console.log("EPSG_code = " + UTM_zone_to_epsg(utm_zone));
  const utm_xy = proj4(UTM_zone_to_epsg(utm_zone)).forward([center_latLng.lng, center_latLng.lat]);
  if (center_latLng.lat < 0) { xy_y += 10000000.0; }

  // ====== utm_odometry_nodeの形式に合わせたフレームIDの作成 ======
  const utm_frame_id = `utm/utm_${utm_zone}Z`;

  // geolonia の高度取得APIを使用して、指定座標のジオイド高＋標高＋地上高を計算.
  // https://blog.geolonia.com/2023/04/14/geoid-api.html
  const Http = new XMLHttpRequest();
  const url = `https://api-vt.geolonia.com/api/altitude?lat=${center_latLng.lat}&lng=${center_latLng.lng}`;
  Http.open("GET", url);
  Http.send();
  Http.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      console.log(Http.responseText);

      //geoloniaから取得したJSON形式テキストをjavascriptオブジェクトに変換.
      const alt_obj = JSON.parse(Http.responseText);

      //ジオイド高(geoid),標高(altitude),地上高(ground_height)
      const pub_altitude = Number(alt_obj.geoid) + Number(alt_obj.altitude) + Number(ground_height);
      console.log(`publish altitude = ${pub_altitude}`);

      // publishメッセージの作成.
      let second_pose = new ROSLIB.Message({
        header: {
          frame_id: utm_frame_id
        },
        child_frame_id: '',
        pose: {
          // 姿勢情報
          pose: {
            position: {
              x: utm_xy[0],
              y: utm_xy[1],
              z: pub_altitude
            }
          },
          covariance: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
        }
      });
      // publish
      gps_second_pub.publish(second_pose);
    }
  }
}
