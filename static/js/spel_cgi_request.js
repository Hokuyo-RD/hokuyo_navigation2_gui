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

  fetch('http://'+SPEL_IP+'/cgi-bin/call_start_spel.bash')
    .then(response => response.text())
    .then(data => {
      console.log('サーバーからの応答:', data);
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

  fetch('http://'+SPEL_IP+'/cgi-bin/call_stop_spel.bash')
    .then(response => response.text())
    .then(data => {
      console.log('サーバーからの応答:', data);
      appendLog(logElement, data);
    });
}

function set_domain_id() {
  showLogContainer();
  const logElement = document.getElementById('command_processing');
  if (logElement) logElement.innerText = "";
  appendLog(logElement, "SET DOMAIN ID...");

  const ros_domain_id = document.getElementById("ros_domain_id").value;

  fetch('http://'+SPEL_IP+'/cgi-bin/call_set_id.bash?ros_domain_id=' + encodeURIComponent(ros_domain_id))
    .then(response => response.text())
    .then(data => {
      console.log('サーバーからの応答:', data);
      appendLog(logElement, data);
    });
}


function get_spel_state(callback) {
  if (!callback) showLogContainer();
  const logElement = document.getElementById('command_processing');
  if (logElement && !callback) { // 状態表示の更新時はログをクリアしない
    logElement.innerText = "";
    appendLog(logElement, "GET SPEL STATE...");
  }

  fetch('http://'+SPEL_IP+'/cgi-bin/call_get_state.bash')
    .then(response => response.text())
    .then(data => {
      console.log('SPEL STATE:', data);
      SPEL_STATE=data;
      if (callback) callback(data.trim());
      else appendLog(logElement, SPEL_STATE);
    });
}
