
const sensorList = {
    UAM1: 0,
    UAM2: 1,
    UST1: 2,
    UST2: 3,
    YVT: 4,
    YLM: 5,
    GNSS: 6
}

//センサ名のリスト
const sensorNameList = [
    "UAM1", "UAM2", "UST1", "UST2", "YVT", "YLM", "GNSS"
]

class SensorStateAdmin {
    constructor() { }
    sensorStateList = [
        true,
        true,
        true,
        true,
        true,
        true,
        true
    ]

    //センサ状態をセットする（bool）
    setSensorState({
        UAM1: UAM1,
        UAM2: UAM2,
        UST1: UST1,
        UST2: UST2,
        YVT: YVT,
        YLM: YLM,
        GNSS: GNSS
    }) {
        this.sensorStateList =
        [
            UAM1,
            UAM2,
            UST1,
            UST2,
            YVT,
            YLM,
            GNSS
        ]
    };

    generateSensorStateTable(){
        var ret = "<div class = \"sensor_table\"><table><tr><th>センサ</th><th>通信状態</th></tr>" ;
        for (var i = 0; i < this.sensorStateList.length;i++){
            var state = this.sensorStateList[i]?"正常（true）":"エラー（false）";
            ret += "<tr><td>"+sensorNameList[i]+"</td>"+"<td>"+ state +"</td></tr>";
        }
        ret += "</table></div>";
        return ret;
    };

    show() {
        swal.fire({
            title: 'センサ接続状態',
            html: this.generateSensorStateTable()
        }
        );
    };
}