console.log("higobashi読み込み済");


//肥後橋にマーカー追加するだけ.
function mark_higobashi(){
  var higobashi_plane = [-46028,-145141];
  
  //6系のEPSGコードを取得.
  var firstProjection = sysnum2epsg(6)
  
  var higobashi_world = proj4(firstProjection).inverse(higobashi_plane);
  
  //世界座標から逆算(不使用)
  //var re_higobashi_plane = proj4(firstProjection).forward(higobashi_world);
  
  //マーカー作成.
  
  var marker = L.marker([higobashi_world[1], higobashi_world[0]]).addTo(map).on('click', (e) => {
      mk_three();
  });
  //var marker = L.marker(higobashi_world).addTo(map);
  document.getElementById("testarea").textContent = firstProjection + "[ " + higobashi_world[1].toString() + " , " + higobashi_world[0].toString()+" ]";
  
}