//ポップアップをodomが送られてくるたびに更新すると描画がカクつくので、表示に間隔を設ける。
const popupDelay = 10;
const audio_array = [
    new Audio("./Audio/L.wav"),     //001 -1
    new Audio("./Audio/M.wav"),     //010 -1
    new Audio("./Audio/LM.wav"),    //011 -1
    new Audio("./Audio/S.wav"),     //100 -1
    new Audio("./Audio/LS.wav"),    //101 -1
    new Audio("./Audio/MS.wav"),    //110 -1
    new Audio("./Audio/LMS.wav")    //111 -1
];
var last_audio_id;

class LostPropAdmin {
    constructor() {
        this.foundProps = [
            { layer: new LayerControlAdmin("myakuMyaku1", true, []), id: 0 },
            { layer: new LayerControlAdmin("myakuMyaku2", true, []), id: 1 },
            { layer: new LayerControlAdmin("myakuMyaku3", true, []), id: 2 }];
    };
    foundProps;
    count = 0;
    popup;
    ///落とし物のポップアップを表示
    showPopup = function (latlng) {
        if (this.isEmpty()) {
            map.closePopup();
            this.popup = null;
            return;
        }
        if (this.count < popupDelay) {
            this.count++;
            return
        }
        else {
            this.count = 0;
        }
        var images = '';
        var audio_id = 0;
        this.foundProps.forEach(
            prop => {
                if (prop.layer.markers.length != 0) {
                    images += `<img src="Image/banpaku_lost${prop.id}.png" width = "70" alt = "Image"}">`;
                    audio_id += 2 ** prop.id;
                }
            });
        //if(audio_id != 0){
        if(audio_id != last_audio_id){
            audio_array[audio_id -1].play();
        }

        if (this.popup == null) {
            this.popup = L.popup({ offset: L.point(0, -30), closeButton: false, autoClose: false, closeOnClick: false }).setLatLng(latlng).setContent(`<button class='popup-content' id ='popup'> 落とし物を発見しました！</br></br> ${images}</div>`);
            var props = this.foundProps;
            //クリックイベントを設定
            map.once('popupopen', function () {
                const btn = document.getElementById('popup');
                if (btn) {
                    btn.addEventListener('click', { props: props, handleEvent: showLostDialog })
                }
            });

            this.popup.openOn(map);
        }
        else {
            var props = this.foundProps;
            this.popup.setLatLng(latlng);
            this.popup.setContent(`<button class='popup-content' id ='popup'> 落とし物を発見しました！</br></br> ${images}</div>`);
            const btn = document.getElementById('popup');
            if (btn) {
                btn.addEventListener('click', { props: props, handleEvent: showLostDialog })
            }
        }
    }
    visibleChanged(checked) {
        for (var prop of this.foundProps) {
            prop.layer.visibleChanged(checked);
        }
    }

    //落とし物が空ならtrueを返す
    isEmpty = function () {
        for (var prop of this.foundProps) {
            if (!prop.layer.markers.length != true) {
                return false;
            }
        }
        return true;
    }

    //すべてのマーカーを削除
    deleteAllMarker = function () {
        for (var prop in this.foundProps) {
            prop.layer.deleteAllMarker();
        }
    }
    //マーカーを追加
    addMarker = function (id, marker) {
        if (id < 0 || id > this.foundProps.length) {
            return;
        }
        if (this.foundProps[id].layer.markers.length == 0) { this.foundProps[id].layer.addToLayer(marker); }
    }
}



showLostDialog = function (e) {
    if (!swal.isVisible()) {
        swal.fire({
            title: "発見した落とし物",
            html: '<!DOCTYPE html><html lang="ja"><head>  <meta charset="UTF-8"><style>    body {      font-family: sans-serif;      padding: 20px;      text-align: center;      background-color: #f0f0f0;    }  .gallery {      display: flex;      justify-content: center;      gap: 20px;    flex-wrap: nowrap;      }     .image-card {   display:flex; flex-direction:column; align-items:center background: #fff; padding: 15px;      border-radius: 10px;      box-shadow: 0 4px 10px rgba(0,0,0,0.1);      width: 200px;    }     .image-card img {      width: 100%;      height: auto;      border-radius: 6px;      transition: filter 0.3s;    }     .image-card button {      margin-top: 10px;      padding: 8px 16px;      border: none;      background-color: #007bff;      color: white;      border-radius: 4px;      cursor: pointer;      font-size: 1em;    }     .image-card button:hover {      background-color: #0056b3;    }     .grayscale {      filter: grayscale(100%) opacity(0.3);;    }  </style></head><body> <div class="gallery" id="gallery">    <!-- JavaScriptで動的に画像を追加 -->  </div>    </body></html>',
            width: '50%',
        })
    }
    // インスタンス化してUIを初期化
    const selector = new ImageSelector("gallery", this.props);
    //　選択された画像だけ選択状態に
    this.props.forEach(prop => {
        if (prop.layer.markers.length != 0) {
            selector.setSelected(prop.id);
        }
    });
}

class ImageSelector {
    lostProps;
    constructor(containerId, props) {
        this.container = document.getElementById(containerId);
        this.images = [{ src: "Image/banpaku_lost0.png", id: 0 },
        { src: "Image/banpaku_lost1.png", id: 1 },
        { src: "Image/banpaku_lost2.png", id: 2 }];
        this.imageElements = {};
        this.render();
        this.lostProps = props;

        //　選択された画像だけ選択状態に
        this.lostProps.forEach(prop => {
            if (prop.layer.markers.length != 0) {
                this.setSelected(prop.id);
            }
        });
    }
    render() {
        this.images.forEach(
            image => {
                const card = document.createElement("div");
                card.className = "image-card";
                const img = document.createElement("img");
                img.src = image.src;
                img.alt = `Image ${image.id}`;
                img.id = `image-${image.id}`;
                this.imageElements[image.id] = img;
                const button = document.createElement("button");
                button.innerText = `回収しました`;
                this.setUnselected(image.id);
                button.onclick = () => {
                    this.setUnselected(image.id);
                    this.lostProps[image.id].layer.deleteAllMarker();
                };
                card.classList.add("gallery");
                card.appendChild(img);
                card.appendChild(button);
                this.container.appendChild(card);
            });
    }       // 非選択状態（白黒）
    setUnselected(id) {
        const img = this.imageElements[id];
        if (img) {
            img.classList.add("grayscale");
        }
    }       // 選択状態（カラー）
    setSelected(id) {
        const img = this.imageElements[id];
        if (img) {
            img.classList.remove("grayscale");
        }
    }
}