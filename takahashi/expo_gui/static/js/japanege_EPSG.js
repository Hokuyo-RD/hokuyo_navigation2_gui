
console.log("JS読み込み済");

function sysnum2epsg(num){
    switch (num){
        case 1:
          return "EPSG:6669";
        case 2:
          return "EPSG:6670";
        case 3:
          return "EPSG:6671";
        case 4:
          return "EPSG:6672";          
        case 5:
          return "EPSG:6673";
        case 6:
          return "EPSG:6674";
        case 7:
          return "EPSG:6675";
        case 8:
          return "EPSG:6676";
        case 9:
          return "EPSG:6677";
        case 10:
          return "EPSG:6678";
        case 11:
          return "EPSG:6679";
        case 12:
          return "EPSG:6680";
        case 13:
          return "EPSG:6681";
        case 14:
          return "EPSG:6682";
        case 15:
          return "EPSG:6683";
        case 16:
          return "EPSG:6684";
        case 17:
          return "EPSG:6685";
        case 18:
          return "EPSG:6686";
        case 19:
          return "EPSG:6687";
        default:
          return "EPSG:4301";
    }
}


//リソースをすべて読み込んだ後に、proj4jsに追記.
window.addEventListener('load', function(){
  add2proj4_JP();
});

function add2proj4_JP(){
  proj4.defs([
      //1
      ["EPSG:6669","+proj=tmerc +lat_0=33 +lon_0=129.5 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //2
      ["EPSG:6670","+proj=tmerc +lat_0=33 +lon_0=131 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //3
      ["EPSG:6671","+proj=tmerc +lat_0=36 +lon_0=132.166666666667 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //4
      ["EPSG:6672","+proj=tmerc +lat_0=33 +lon_0=133.5 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //5
      ["EPSG:6673","+proj=tmerc +lat_0=36 +lon_0=134.333333333333 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //6
      ["EPSG:6674","+proj=tmerc +lat_0=36 +lon_0=136 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //7
      ["EPSG:6675","+proj=tmerc +lat_0=36 +lon_0=137.166666666667 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //8
      ["EPSG:6676","+proj=tmerc +lat_0=36 +lon_0=138.5 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //9
      ["EPSG:6677","+proj=tmerc +lat_0=36 +lon_0=139.833333333333 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //10
      ["EPSG:6678","+proj=tmerc +lat_0=40 +lon_0=140.833333333333 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //11
      ["EPSG:6679","+proj=tmerc +lat_0=44 +lon_0=140.25 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //12
      ["EPSG:6680","+proj=tmerc +lat_0=44 +lon_0=142.25 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //13
      ["EPSG:6681","+proj=tmerc +lat_0=44 +lon_0=144.25 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //14
      ["EPSG:6682","+proj=tmerc +lat_0=26 +lon_0=142 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //15
      ["EPSG:6683","+proj=tmerc +lat_0=26 +lon_0=127.5 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //16
      ["EPSG:6684","+proj=tmerc +lat_0=26 +lon_0=124 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //17
      ["EPSG:6685","+proj=tmerc +lat_0=26 +lon_0=131 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //18
      ["EPSG:6686","+proj=tmerc +lat_0=20 +lon_0=136 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      //19
      ["EPSG:6687","+proj=tmerc +lat_0=26 +lon_0=154 +k=0.9999 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
  
      ["EPSG:4301", //東京測地系/日本測地系 SRID=4301
          "+proj=longlat +ellps=bessel +towgs84=-146.414,507.337,680.507,0,0,0,0 +no_defs"
      ],
      ["EPSG:3097","+proj=utm +zone=51 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      ["EPSG:3098","+proj=utm +zone=52 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      ["EPSG:3099","+proj=utm +zone=53 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      ["EPSG:3100","+proj=utm +zone=54 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      ["EPSG:3101","+proj=utm +zone=55 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      ["EPSG:6690","+proj=utm +zone=53 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs"],
      ["EPSG:32653","+proj=utm +zone=53 +datum=WGS84 +units=m +no_defs +type=crs"],
      ["EPSG:32753","+proj=utm +zone=53 +south +datum=WGS84 +units=m +no_defs +type=crs"]
      
  ]);
}


