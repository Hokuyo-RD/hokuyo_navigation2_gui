
//ドラッグでアイコンを追加できるコントロール
const DragControl = L.Control.extend({

  onAdd: function (map) {

    const div = L.DomUtil.create('div', 'custom-control');

    div.innerHTML = "ウェイポイント";

    L.DomEvent.disableClickPropagation(div);

    div.addEventListener('mousedown', startDrag);
    div.addEventListener('touchstart', startDrag, { passive: false });

    return div;

  }

});

const dragIcon = document.getElementById('dragIcon');

let isDragging = false;

let lastTouchedPosition = null;

function getEventPosition(e) {
  if (e.touches) {
    const pos = {
      x: e.touches[0].pageX,
      y: e.touches[0].pageY
    };
    lastTouchedPosition = pos;
    return pos;
  } else {
    return {
      x: e.pageX,
      y: e.pageY
    };
  }
}

function startDrag(e) {

  e.preventDefault(); // スクロールなどを防止

  isDragging = true;

  dragIcon.style.display = 'block';
  const pos = getEventPosition(e);

  moveIconInternal(pos);
  document.addEventListener('mousemove', onMove);
  document.addEventListener('mouseup', onEnd);
  document.addEventListener('touchmove', onMove, { passive: false });
  document.addEventListener('touchend', onEnd);
}



function onMove(e) {
  e.preventDefault();
  const pos = getEventPosition(e);
  moveIconInternal(pos);
}



function moveIconInternal(pos) {
  dragIcon.style.left = `${pos.x - 12}px`;
  dragIcon.style.top = `${pos.y - 41}px`;
}



function onEnd(e) {
  isDragging = false;
  dragIcon.style.display = 'none';

  document.removeEventListener('mousemove', onMove);
  document.removeEventListener('mouseup', onEnd);
  document.removeEventListener('touchmove', onMove);
  document.removeEventListener('touchend', onEnd);

  var pos;
  if (e.type == 'touchend' && lastTouchedPosition) {
    pos = lastTouchedPosition;
  }
  else if (e.type == 'mouseup') {
    pos = getEventPosition(e);
  }
  else { return }
  const rect = map.getContainer().getBoundingClientRect();
  const x = pos.x - rect.left;
  const y = pos.y - rect.top;
  const latlng = map.containerPointToLatLng([x, y]);

  if (wayPointMarker) {
    map.removeLayer(wayPointMarker);
  }
  wayPointMarker = L.marker(latlng).addTo(map);
}

const wayPointButton = document.getElementById('expoWayPointButton');
const modalDialog = document.getElementById('modalDialog');

// モーダルを開く
wayPointButton?.addEventListener('click', async () => {
  if (wayPointMarker == null) {
    alert("ウェイポイントが置かれていません。\nマップ上にウェイポイントを置いてください。");
    return;
  }
  document.getElementById("modalMessage").textContent = "ウェイポイントを送信しますか？\r\n座標：（ 緯度 " + wayPointMarker.getLatLng().lat + " , 経度 " + wayPointMarker.getLatLng().lng + " ）";
  modalDialog.showModal();
  // モーダルダイアログを表示する際に背景部分がスクロールしないようにする
  document.documentElement.style.overflow = "hidden";
});

const closeButton = document.getElementById('closeButton');

// モーダルを閉じる
closeButton?.addEventListener('click', async () => {
  modalDialog.close();
  // モーダルを解除すると、スクロール可能になる
  document.documentElement.removeAttribute("style");
});

const okButton = document.getElementById('okButton');

okButton?.addEventListener('click', async () => {
  SendWayPoint();
  modalDialog.close();
  alert("ウェイポイントを送信しました。");
  // モーダルを解除すると、スクロール可能になる
  document.documentElement.removeAttribute("style");
});

///ウェイポイントを送信し、マーカーを削除する。
function SendWayPoint() {
  pub_expo_wayPoint();
  map.removeLayer(wayPointMarker);
  wayPointMarker = null;
}