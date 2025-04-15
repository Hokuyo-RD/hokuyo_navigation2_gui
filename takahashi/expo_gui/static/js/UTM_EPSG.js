console.log("UTM_EPSG 読み込み済");

// 経度からUTMゾーンを取得する関数.
function judge_UTM_zone(longitude){
  //経度を6で割って切り上げ.
  const zone = Math.ceil( (longitude + 180.0)/6 );
  return zone;
}

function UTM_zone_to_epsg(num){
  //UTM 1~60 :: 32601 ~ 32660
  return `EPSG:${num+32600}`;
}


//リソースをすべて読み込んだ後に、proj4jsに追記.
window.addEventListener('load', function(){
  add_UTM_to_proj4js();
});

// https://epsg.io/32701.proj4js
//UTM 1N~60N :: 32601 ~ 32660
//UTM 1S~60S :: 32701 ~ 32760
function add_UTM_to_proj4js(){
  proj4.defs([
    //1N
    ["EPSG:32601","+proj=utm +zone=1 +datum=WGS84 +units=m +no_defs +type=crs"],
    //1S
    ["EPSG:32701","+proj=utm +zone=1 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //2N
    ["EPSG:32602","+proj=utm +zone=2 +datum=WGS84 +units=m +no_defs +type=crs"],
    //2S
    ["EPSG:32702","+proj=utm +zone=2 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //3N
    ["EPSG:32603","+proj=utm +zone=3 +datum=WGS84 +units=m +no_defs +type=crs"],
    //3S
    ["EPSG:32703","+proj=utm +zone=3 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //4N
    ["EPSG:32604","+proj=utm +zone=4 +datum=WGS84 +units=m +no_defs +type=crs"],
    //4S
    ["EPSG:32704","+proj=utm +zone=4 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //5N
    ["EPSG:32605","+proj=utm +zone=5 +datum=WGS84 +units=m +no_defs +type=crs"],
    //5S
    ["EPSG:32705","+proj=utm +zone=5 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //6N
    ["EPSG:32606","+proj=utm +zone=6 +datum=WGS84 +units=m +no_defs +type=crs"],
    //6S
    ["EPSG:32706","+proj=utm +zone=6 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //7N
    ["EPSG:32607","+proj=utm +zone=7 +datum=WGS84 +units=m +no_defs +type=crs"],
    //7S
    ["EPSG:32707","+proj=utm +zone=7 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //8N
    ["EPSG:32608","+proj=utm +zone=8 +datum=WGS84 +units=m +no_defs +type=crs"],
    //8S
    ["EPSG:32708","+proj=utm +zone=8 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //9N
    ["EPSG:32609","+proj=utm +zone=9 +datum=WGS84 +units=m +no_defs +type=crs"],
    //9S
    ["EPSG:32709","+proj=utm +zone=9 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //10N
    ["EPSG:32610","+proj=utm +zone=10 +datum=WGS84 +units=m +no_defs +type=crs"],
    //10S
    ["EPSG:32710","+proj=utm +zone=10 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //11N
    ["EPSG:32611","+proj=utm +zone=11 +datum=WGS84 +units=m +no_defs +type=crs"],
    //11S
    ["EPSG:32711","+proj=utm +zone=11 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //12N
    ["EPSG:32612","+proj=utm +zone=12 +datum=WGS84 +units=m +no_defs +type=crs"],
    //12S
    ["EPSG:32712","+proj=utm +zone=12 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //13N
    ["EPSG:32613","+proj=utm +zone=13 +datum=WGS84 +units=m +no_defs +type=crs"],
    //13S
    ["EPSG:32713","+proj=utm +zone=13 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //14N
    ["EPSG:32614","+proj=utm +zone=14 +datum=WGS84 +units=m +no_defs +type=crs"],
    //14S
    ["EPSG:32714","+proj=utm +zone=14 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //15N
    ["EPSG:32615","+proj=utm +zone=15 +datum=WGS84 +units=m +no_defs +type=crs"],
    //15S
    ["EPSG:32715","+proj=utm +zone=15 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //16N
    ["EPSG:32616","+proj=utm +zone=16 +datum=WGS84 +units=m +no_defs +type=crs"],
    //16S
    ["EPSG:32716","+proj=utm +zone=16 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //17N
    ["EPSG:32617","+proj=utm +zone=17 +datum=WGS84 +units=m +no_defs +type=crs"],
    //17S
    ["EPSG:32717","+proj=utm +zone=17 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //18N
    ["EPSG:32618","+proj=utm +zone=18 +datum=WGS84 +units=m +no_defs +type=crs"],
    //18S
    ["EPSG:32718","+proj=utm +zone=18 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //19N
    ["EPSG:32619","+proj=utm +zone=19 +datum=WGS84 +units=m +no_defs +type=crs"],
    //19S
    ["EPSG:32719","+proj=utm +zone=19 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //20N
    ["EPSG:32620","+proj=utm +zone=20 +datum=WGS84 +units=m +no_defs +type=crs"],
    //20S
    ["EPSG:32720","+proj=utm +zone=20 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //21N
    ["EPSG:32621","+proj=utm +zone=21 +datum=WGS84 +units=m +no_defs +type=crs"],
    //21S
    ["EPSG:32721","+proj=utm +zone=21 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //22N
    ["EPSG:32622","+proj=utm +zone=22 +datum=WGS84 +units=m +no_defs +type=crs"],
    //22S
    ["EPSG:32722","+proj=utm +zone=22 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //23N
    ["EPSG:32623","+proj=utm +zone=23 +datum=WGS84 +units=m +no_defs +type=crs"],
    //23S
    ["EPSG:32723","+proj=utm +zone=23 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //24N
    ["EPSG:32624","+proj=utm +zone=24 +datum=WGS84 +units=m +no_defs +type=crs"],
    //24S
    ["EPSG:32724","+proj=utm +zone=24 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //25N
    ["EPSG:32625","+proj=utm +zone=25 +datum=WGS84 +units=m +no_defs +type=crs"],
    //25S
    ["EPSG:32725","+proj=utm +zone=25 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //26N
    ["EPSG:32626","+proj=utm +zone=26 +datum=WGS84 +units=m +no_defs +type=crs"],
    //26S
    ["EPSG:32726","+proj=utm +zone=26 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //27N
    ["EPSG:32627","+proj=utm +zone=27 +datum=WGS84 +units=m +no_defs +type=crs"],
    //27S
    ["EPSG:32727","+proj=utm +zone=27 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //28N
    ["EPSG:32628","+proj=utm +zone=28 +datum=WGS84 +units=m +no_defs +type=crs"],
    //28S
    ["EPSG:32728","+proj=utm +zone=28 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //29N
    ["EPSG:32629","+proj=utm +zone=29 +datum=WGS84 +units=m +no_defs +type=crs"],
    //29S
    ["EPSG:32729","+proj=utm +zone=29 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //30N
    ["EPSG:32630","+proj=utm +zone=30 +datum=WGS84 +units=m +no_defs +type=crs"],
    //30S
    ["EPSG:32730","+proj=utm +zone=30 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //31N
    ["EPSG:32631","+proj=utm +zone=31 +datum=WGS84 +units=m +no_defs +type=crs"],
    //31S
    ["EPSG:32731","+proj=utm +zone=31 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //32N
    ["EPSG:32632","+proj=utm +zone=32 +datum=WGS84 +units=m +no_defs +type=crs"],
    //32S
    ["EPSG:32732","+proj=utm +zone=32 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //33N
    ["EPSG:32633","+proj=utm +zone=33 +datum=WGS84 +units=m +no_defs +type=crs"],
    //33S
    ["EPSG:32733","+proj=utm +zone=33 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //34N
    ["EPSG:32634","+proj=utm +zone=34 +datum=WGS84 +units=m +no_defs +type=crs"],
    //34S
    ["EPSG:32734","+proj=utm +zone=34 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //35N
    ["EPSG:32635","+proj=utm +zone=35 +datum=WGS84 +units=m +no_defs +type=crs"],
    //35S
    ["EPSG:32735","+proj=utm +zone=35 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //36N
    ["EPSG:32636","+proj=utm +zone=36 +datum=WGS84 +units=m +no_defs +type=crs"],
    //36S
    ["EPSG:32736","+proj=utm +zone=36 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //37N
    ["EPSG:32637","+proj=utm +zone=37 +datum=WGS84 +units=m +no_defs +type=crs"],
    //37S
    ["EPSG:32737","+proj=utm +zone=37 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //38N
    ["EPSG:32638","+proj=utm +zone=38 +datum=WGS84 +units=m +no_defs +type=crs"],
    //38S
    ["EPSG:32738","+proj=utm +zone=38 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //39N
    ["EPSG:32639","+proj=utm +zone=39 +datum=WGS84 +units=m +no_defs +type=crs"],
    //39S
    ["EPSG:32739","+proj=utm +zone=39 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //40N
    ["EPSG:32640","+proj=utm +zone=40 +datum=WGS84 +units=m +no_defs +type=crs"],
    //40S
    ["EPSG:32740","+proj=utm +zone=40 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //41N
    ["EPSG:32641","+proj=utm +zone=41 +datum=WGS84 +units=m +no_defs +type=crs"],
    //41S
    ["EPSG:32741","+proj=utm +zone=41 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //42N
    ["EPSG:32642","+proj=utm +zone=42 +datum=WGS84 +units=m +no_defs +type=crs"],
    //42S
    ["EPSG:32742","+proj=utm +zone=42 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //43N
    ["EPSG:32643","+proj=utm +zone=43 +datum=WGS84 +units=m +no_defs +type=crs"],
    //43S
    ["EPSG:32743","+proj=utm +zone=43 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //44N
    ["EPSG:32644","+proj=utm +zone=44 +datum=WGS84 +units=m +no_defs +type=crs"],
    //44S
    ["EPSG:32744","+proj=utm +zone=44 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //45N
    ["EPSG:32645","+proj=utm +zone=45 +datum=WGS84 +units=m +no_defs +type=crs"],
    //45S
    ["EPSG:32745","+proj=utm +zone=45 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //46N
    ["EPSG:32646","+proj=utm +zone=46 +datum=WGS84 +units=m +no_defs +type=crs"],
    //46S
    ["EPSG:32746","+proj=utm +zone=46 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //47N
    ["EPSG:32647","+proj=utm +zone=47 +datum=WGS84 +units=m +no_defs +type=crs"],
    //47S
    ["EPSG:32747","+proj=utm +zone=47 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //48N
    ["EPSG:32648","+proj=utm +zone=48 +datum=WGS84 +units=m +no_defs +type=crs"],
    //48S
    ["EPSG:32748","+proj=utm +zone=48 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //49N
    ["EPSG:32649","+proj=utm +zone=49 +datum=WGS84 +units=m +no_defs +type=crs"],
    //49S
    ["EPSG:32749","+proj=utm +zone=49 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //50N
    ["EPSG:32650","+proj=utm +zone=50 +datum=WGS84 +units=m +no_defs +type=crs"],
    //50S
    ["EPSG:32750","+proj=utm +zone=50 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //51N
    ["EPSG:32651","+proj=utm +zone=51 +datum=WGS84 +units=m +no_defs +type=crs"],
    //51S
    ["EPSG:32751","+proj=utm +zone=51 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //52N
    ["EPSG:32652","+proj=utm +zone=52 +datum=WGS84 +units=m +no_defs +type=crs"],
    //52S
    ["EPSG:32752","+proj=utm +zone=52 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //53N
    ["EPSG:32653","+proj=utm +zone=53 +datum=WGS84 +units=m +no_defs +type=crs"],
    //53S
    ["EPSG:32753","+proj=utm +zone=53 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //54S
    ["EPSG:32754","+proj=utm +zone=54 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //54N
    ["EPSG:32654","+proj=utm +zone=54 +datum=WGS84 +units=m +no_defs +type=crs"],
    //55N
    ["EPSG:32655","+proj=utm +zone=55 +datum=WGS84 +units=m +no_defs +type=crs"],
    //55S
    ["EPSG:32755","+proj=utm +zone=55 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //56N
    ["EPSG:32656","+proj=utm +zone=56 +datum=WGS84 +units=m +no_defs +type=crs"],
    //56S
    ["EPSG:32756","+proj=utm +zone=56 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //57N
    ["EPSG:32657","+proj=utm +zone=57 +datum=WGS84 +units=m +no_defs +type=crs"],
    //57S
    ["EPSG:32757","+proj=utm +zone=57 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //58N
    ["EPSG:32658","+proj=utm +zone=58 +datum=WGS84 +units=m +no_defs +type=crs"],
    //58S
    ["EPSG:32758","+proj=utm +zone=58 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //59N
    ["EPSG:32659","+proj=utm +zone=59 +datum=WGS84 +units=m +no_defs +type=crs"],
    //59S
    ["EPSG:32759","+proj=utm +zone=59 +south +datum=WGS84 +units=m +no_defs +type=crs"],
    //60N
    ["EPSG:32660","+proj=utm +zone=60 +datum=WGS84 +units=m +no_defs +type=crs"],
    //60S
    ["EPSG:32760","+proj=utm +zone=60 +south +datum=WGS84 +units=m +no_defs +type=crs"]
    
  ]);
}


