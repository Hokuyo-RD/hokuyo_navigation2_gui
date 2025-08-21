var connection = "";

//コネクション開始ボタン

function open_cnn(){

  console.log("コネクションを開始しします。");

  connection = new WebSocket('wss://echo.websocket.org');

  //コネクションが接続された時の動き

  connection.onopen = function(e) {

    console.log("コネクションを開始しまいた。");

  };

  //エラーが発生したされた時の動き

  connection.onerror = function(error) {

    console.log("エラーが発生しました。");

  };

  //メッセージを受け取ったされた時の動き

  connection.onmessage = function(e) {

    let msg = "メッセージを受信しました。" + e.data;

    document.getElementById("RcvMsg").value = msg;

  };

  //通信が切断された時の動き

  connection.onclose = function() {

    console.log("コネクションを終了しまいた。");

  };

}

//メッセージ送信ボタン

function snd_msg(){

  connection.send(document.getElementById("SndMsg").value);

}

//コネクション終了ボタン

function close_cnn(){

  connection.close();

}