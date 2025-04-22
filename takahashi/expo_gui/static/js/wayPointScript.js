
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
    if (e.touches ) {
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
    if(e.type == 'touchend' && lastTouchedPosition){
      pos = lastTouchedPosition;
    }
    else if(e.type == 'mouseup'){
      pos = getEventPosition(e);
    }
    else{return}
    const rect = map.getContainer().getBoundingClientRect();
    const x = pos.x - rect.left;
    const y = pos.y - rect.top;
    const latlng = map.containerPointToLatLng([x, y]);
  
    if (wayPointMarker) {
      map.removeLayer(wayPointMarker);
    }
    wayPointMarker = L.marker(latlng).addTo(map);
  }
  
  