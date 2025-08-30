class ExpoBoundsAdmin{
  constructor(){
    
  }

}

function generateTileGroup(startLatLng, widthPerCell, lengthPerCell, numberOfRows, numberOfLines) {
  var crowding_polygones = [];
 var tempStartPoint = startLatLng;
 var tempCell;

  for (var i = 1; i < numberOfRows; i++) {
    for (var j = 1; j < numberOfLines; j++) {
      tempCell = L.latLngBounds(
        [tempStartPoint,
          [tempStartPoint[0] +widthPerCell,tempStartPoint[1] + lengthPerCell]]
      );
      crowding_polygones.push(L.rectangle(tempCell, { color: "#dcdcdc", weight: 5, fill: true, fillColor: "#dcdcdc", opacity: 0.3 }).addTo(map));
  
      tempStartPoint = [tempStartPoint[0],tempStartPoint[1]+lengthPerCell];
    }
    tempStartPoint = [startLatLng[0] + widthPerCell * i, startLatLng[1]];
  };

  return new LayerControlAdmin("crowd", true, crowding_polygones);
};