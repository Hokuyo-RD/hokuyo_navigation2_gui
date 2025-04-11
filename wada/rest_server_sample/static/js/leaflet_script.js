    // マップを作る
    var map = L.map("mapid",{rotate:true }).setView([34.69176319251114, 135.49633723119732], 16);

    
    // マップの設定

    //国土地理院航空写真.
    
    L.tileLayer('https://cyberjapandata.gsi.go.jp/xyz/seamlessphoto/{z}/{x}/{y}.jpg', {
        maxNativeZoom: 18,
        maxZoom: 25,
        attribution: "<a href='https://maps.gsi.go.jp/development/ichiran.html' target='_blank'>国土地理院</a>",
    }).addTo(map);
    

    //OpenStreetMap
    /*
    L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxNativeZoom: 18,
        maxZoom: 25,
        attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
    }).addTo(map);
    */

    //MapBox
    /*
    //L.tileLayer('https://api.tiles.mapbox.com/v4/{id}/{z}/{x}/{y}.png?access_token={accessToken}', {
    L.tileLayer('https://api.mapbox.com/styles/v1/mapbox/{id}/tiles/{z}/{x}/{y}?access_token={accessToken}', {
        attribution: 'Map data &copy; <a href="http://openstreetmap.org">OpenStreetMap</a> contributors, <a href="http://creativecommons.org/licenses/by-sa/2.0/">CC-BY-SA</a>, Imagery © <a href="http://mapbox.com">Mapbox</a>',
        maxNativeZoom: 18,
        maxZoom: 25,//id: 'mapbox.streets',
        id: 'light-v10',
        accessToken: 'pk.eyJ1IjoidC1va2Ftb3RvIiwiYSI6ImNsc3UyM2h3NzA4bGIya28xbmNkOXBmYnEifQ.fnsbHf6hfQiH5xJ-v9EOYg'
        //accessToken: <MapBoxのトークンを記載>
    }).addTo(map);
    */



//マーカーアイコンの作成.
  
  // 中心座標のマーカーアイコン.
  const cross = L.divIcon({ // CSSを使ったDivIconを作成
    className: 'cross',
    bgPos: [18, 18]
  });

  //最新点の円マーカーオプション.
  const latestGpsIcon = {
    radius: 6,           //半径
    fillColor: "#ff3322", //塗りつぶし色
    color: "#000000",        //外枠の線の色
    weight: 1,            //外枠の線の太さ
    opacity: 1,           //外枠の線の不透明度
    fillOpacity: 0.8      //塗りつぶしの不透明度
  };
  const latestOdomIcon = {
    radius: 6,           //半径
    fillColor: "#2233ff", //塗りつぶし色
    color: "#000000",        //外枠の線の色
    weight: 1,            //外枠の線の太さ
    opacity: 1,           //外枠の線の不透明度
    fillOpacity: 0.8      //塗りつぶしの不透明度
  };
  const latestfilteredIcon = {
    radius: 6,           //半径
    fillColor: "D3E173", //塗りつぶし色
    color: "#000000",        //外枠の線の色
    weight: 1,            //外枠の線の太さ
    opacity: 1,           //外枠の線の不透明度
    fillOpacity: 0.8      //塗りつぶしの不透明度
  };

  //軌跡のマーカーアイコン.
  const redIcon = L.icon({
    iconUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-icon.png",
    iconRetinaUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-icon-2x.png",
    shadowUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-shadow.png",
    iconSize: [12, 20],
    iconAnchor: [6, 20],
    popupAnchor: [1, -34],
    tooltipAnchor: [16, -28],
    shadowSize: [41, 41],
    className: "icon-red", // <= ここでクラス名を指定
  });
  
  const blueIcon = L.icon({
    iconUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-icon.png",
    iconRetinaUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-icon-2x.png",
    shadowUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-shadow.png",
    iconSize: [12, 20],
    iconAnchor: [6, 20],
    popupAnchor: [1, -34],
    tooltipAnchor: [16, -28],
    shadowSize: [41, 41],
  });

  const selfLocationIcon = L.icon({
    iconUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-icon.png",
    iconRetinaUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-icon-2x.png",
    shadowUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-shadow.png",
    iconSize: [12, 20],
    iconAnchor: [6, 20],
    popupAnchor: [1, -34],
    tooltipAnchor: [16, -28],
    shadowSize: [41, 41],
    className: "icon-selfLocation", // <= ここでクラス名を指定
  });

  const strayIcon = L.icon({
    iconUrl: "icons/stray.png",
    iconRetinaUrl: "https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEhGEswJIZiWi9-2tDf4g6zCkG0AWyPlCZkAhk4ZD0vuD_yHSmH09NQs5kvnOegMoLelVPkYY9Olejiy-jysMbhSV9EsUaPhJVlGumXl5kzqMYer8ryKjbbLgIp5SXYOJLJcG5SuFhfnWY6v/s400/maigo_boy.png",
    shadowUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-shadow.png",
    iconSize: [36, 60],
    iconAnchor: [6, 20],
    popupAnchor: [1, -34],
    tooltipAnchor: [16, -28],
    shadowSize: [41, 41],
    className: "icon-yellow", // <= ここでクラス名を指定
  });
  
  const lostPropIcon = L.icon({
    iconUrl: "icons/lostProp.png",
    iconRetinaUrl: "https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEi_S1V3XToc_t3HZkte-9mLra1Q4SVanpXcTvINOkNcnQbfcGNscwJxidxvNl4ZO8nVw8MXgwamnlaLdzVhWJYFp9FB-7UG6QBRmAH9jlwuj7u87RCiV3px7JMiqfblLK7PZUtEYNiWR0c/s400/wasuremono_box.png",
    shadowUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-shadow.png",
    iconSize: [36, 60],
    iconAnchor: [6, 20],
    popupAnchor: [1, -34],
    tooltipAnchor: [16, -28],
    shadowSize: [41, 41],
    className: "icon-lostProp", // <= ここでクラス名を指定
  });
  const arrowIcon = L.icon({
    iconUrl: "https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEiKRNwRm8OhKM-Td-r63pdwv32DJFhDJrDjpYIOO3XpAQJLrkaHtUus1wKaYmFaHxSmgt9Xwg257gpRfymAtHPGhnrZkUdl7bmvcKGsYpd69qjzE08CQhLn2B-IoHLQqDX4dKfqg7uYJzkx/s800/computer_cursor_arrow_black.png",
    //iconRetinaUrl: "https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEiKRNwRm8OhKM-Td-r63pdwv32DJFhDJrDjpYIOO3XpAQJLrkaHtUus1wKaYmFaHxSmgt9Xwg257gpRfymAtHPGhnrZkUdl7bmvcKGsYpd69qjzE08CQhLn2B-IoHLQqDX4dKfqg7uYJzkx/s800/computer_cursor_arrow_black.png",
    shadowUrl: "https://esm.sh/leaflet@1.9.2/dist/images/marker-shadow.png",
    iconSize: [60, 60],
    iconAnchor: [30, 60],
    popupAnchor: [1, -34],
    tooltipAnchor: [16, -28],
    shadowSize: [41, 41],
    className: "icon-arrow", // <= ここでクラス名を指定
  });
  