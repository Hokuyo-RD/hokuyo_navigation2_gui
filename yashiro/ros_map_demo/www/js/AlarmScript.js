//アラートレベル
const AlarmLevel = {
    LOW: 0,
    MID: 1,
    HIGH: 2,
}
// アラート管理用クラス
class AlarmAdmin {
    constructor(alarmName, dangerMessage) {
        this.alarmLevel = AlarmLevel.LOW;
        var temp = document.getElementsByClassName(alarmName);
        console.log(temp);
        for (var i = 0; i < temp.length; i++) {
            console.log(temp[i]);
        }
        this.alarmColor = document.getElementsByClassName(alarmName)[0];
        this.dangerMessage = dangerMessage;
    }
    alarmLevel;
    alarmColor;
    dangerMessage;
    //アラートレベルを設定（0〜2）
    setAlarmLevel(level) {
        if (level <= 2 || 0 <= level) {
            this.alarmLevel = level;
            this.setBackgroundColor();
            this.popDangerAlert();
        }
    }

    //alertLevel==HIGHならば警告ダイアログを表示
    popDangerAlert() {
        if (this.alarmLevel == AlarmLevel.HIGH) {
            if (!swal.isVisible()) {
                swal.fire({
                    title: "警告",
                    html: this.dangerMessage,
                    icon: "error"
                })
            }
        }
    }

    getAlarmColorCode = function () {
        switch (this.alarmLevel) {
            case AlarmLevel.LOW:
                return "#808080";
            case AlarmLevel.MID:
                return "#ffff00";
            case AlarmLevel.HIGH:
                return "#ff0000";
        }
    }
    setBackgroundColor = function () {
        this.alarmColor.style.setProperty("--bg-color", this.getAlarmColorCode());
    }

    //3秒後にアラートを設定するタイマーを返す。
    setTimer(level) {
        return setInterval(this.setAlarmLevel.bind(this), 3000, level);
    }
}
