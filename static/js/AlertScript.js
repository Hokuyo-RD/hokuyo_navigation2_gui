//アラートレベル
const AlertLevel = {
    LOW: 0,
    MID: 1,
    HIGH: 2,
}
// アラート管理用クラス
class AlertAdmin{
    alertLevel;
    alertColor;
    constructor(alertName){
        this.alertLevel = AlertLevel.LOW;
        this.alertColor = document.getElementsByClassName(alertName)[0];
    }
    //アラートレベルを設定（0〜2）
    setAlertLevel(level){
        if(level <= 2 || 0 <= level){
            this.alertLevel = level;
            this.setBackgroundColor();
        }
    }

    getAlertColorCode = function(){
        switch(this.alertLevel){
            case AlertLevel.LOW:
                return "#808080";
            case AlertLevel.MID:
                return "#ffff00";
            case AlertLevel.HIGH:
                return "#ff0000";
        }
    }
    setBackgroundColor  = function(){
        this.alertColor.style.setProperty("--bg-color", this.getAlertColorCode());
      }
}

