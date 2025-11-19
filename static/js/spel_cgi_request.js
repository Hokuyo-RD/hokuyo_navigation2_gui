/*
SPELのAPI一覧（fetchに渡すURL）：

SPEL起動
http://'+SPEL_IP+'/cgi-bin/call_start_spel.bash

SPEL停止（スペルパソコン上の全ノードをkill）
http://'+SPEL_IP+'/cgi-bin/call_stop_spel.bash

SPELパソコンのROS_DOMAIN_ID設定
http://'+SPEL_IP+'/cgi-bin/call_set_id.bash?ros_domain_id=<任意の整数>

SPELパソコンの現在の状態取得(SPEL or STOP)
http://'+SPEL_IP+'/cgi-bin/call_get_state.bash

*/

let SPEL_STATE='STOP' // SPEL_IPはindex.htmlでグローバルに定義されます

function appendLog(logElement, text) {
  if (!logElement) return;
  const timestamp = new Date().toLocaleTimeString();
  logElement.innerText += `[${timestamp}] ${text}\n`;
  logElement.scrollTop = logElement.scrollHeight; // 自動スクロール
}

function showLogContainer() {
  const logElement = document.getElementById('command_processing');
  const logContainer = logElement ? logElement.closest('.log-output-container') : null;
  if (logContainer) {
    logContainer.style.display = 'block';
  }
}

function start_spel() {
  showLogContainer();
  const logElement = document.getElementById('command_processing');
  if (logElement) logElement.innerText = "";
  appendLog(logElement, "SPEL起動中...");

  fetch('/spel_proxy', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ command: 'start_spel' }),
  })
    .then(response => response.text())
    .then(data => {
      console.log('Server response:', data);
      appendLog(logElement, data);
    });
}

function kill_nodes() {    
  showLogContainer();
  const logElement = document.getElementById('command_processing');
  if (logElement) logElement.innerText = "";
  if (logElement) {
    appendLog(logElement, "SPEL停止中...");
  }

  fetch('/spel_proxy', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ command: 'stop_spel' }),
  })
    .then(response => response.text())
    .then(data => {
      console.log('Server response:', data);
      appendLog(logElement, data);
    });
}

function set_domain_id() {
  showLogContainer();
  const logElement = document.getElementById('command_processing');
  if (logElement) logElement.innerText = "";
  appendLog(logElement, "SET DOMAIN ID...");

  const ros_domain_id = document.getElementById("ros_domain_id").value;

  fetch('/spel_proxy', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ command: 'set_id', ros_domain_id: ros_domain_id }),
  })
    .then(response => response.text())
    .then(data => {
      console.log('Server response:', data);
      appendLog(logElement, data);
    });
}


function get_spel_state(callback) {
  const logElement = document.getElementById('command_processing');
  
  // callbackが指定されている場合は、メイン画面のステータス更新なのでログには何も表示しない
  if (callback) {
    // 何もしない
  } else {
    // callbackがない場合（ボタンクリック時）はログエリアを表示し、ログをクリアしてメッセージを追加
    showLogContainer();
    logElement.innerText = "";
    appendLog(logElement, "GET SPEL STATE...");
  }
  
  fetch('/spel_proxy', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ command: 'get_state' }),
  })
    .then(response => response.text())
    .then(data => {
      console.log('SPEL STATE:', data);
      SPEL_STATE=data;
      // メイン画面のステータス表示を更新する関数を呼び出す
      if (typeof updateSpelStatusDisplay === 'function') updateSpelStatusDisplay(data.trim());
      if (callback) callback(data.trim());
      else appendLog(logElement, SPEL_STATE);
    });
}
