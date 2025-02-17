function mk_three(){
   
    document.getElementById("processing").textContent = 'ここに処理進捗が表示されます。js開始';

    // サイズを指定
    const width = 500;
    const height = 500;

    // レンダラーを作成
    const renderer = new THREE.WebGLRenderer({
      canvas: document.querySelector("#myCanvas"),
    });
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setSize(width, height);

    
    document.getElementById("processing").textContent = 'レンダラー作成';

    // シーンを作成
    const scene = new THREE.Scene();
    document.getElementById("processing").textContent = 'シーン作成';

    // カメラを作成
    const camera = new THREE.PerspectiveCamera(45, width / height);
    let cameraX=0,cameraY=0,cameraZ=10,cameraWZ,cameraWY;
    camera.position.set(cameraX, cameraY, cameraZ);
    document.getElementById("processing").textContent = 'カメラ作成';
    // カメラ移動用コントローラーの作成
    var PCD_canv = document.getElementById('myCanvas');
    let expand = 1,mouseX = 0,mouseY = 0,pre_mouseX=0,pre_mouseY=0,mouse_dif_X=0,mouse_dif_Y=0;
    let LEFTmouseDOWN =false,RIGHTmouseDOWN = false,CENTORmouseDOWN = false;

    //キャンバス内では右クリック時にメニューを表示させない.
    PCD_canv.addEventListener('contextmenu', function(e){
      e.preventDefault();
    });
    
    //押しているマウスボタンの種類を取得.
    PCD_canv.addEventListener('mousedown', function(e) {
      switch (e.button) {
        case 0:LEFTmouseDOWN = true;break;
        case 1:CENTORmouseDOWN = true;break;
        case 2:RIGHTmouseDOWN = true;break;
        default:log.textContent = `不明なボタンコード: ${e.button}`;
      }
    });
    PCD_canv.addEventListener('mouseup', function(e) {
      switch (e.button) {
        case 0:LEFTmouseDOWN = false;break;
        case 1:CENTORmouseDOWN = false;break;
        case 2:RIGHTmouseDOWN = false;break;
        default:log.textContent = `不明なボタンコード: ${e.button}`;
      }
    });
    PCD_canv.addEventListener('wheel', function(e){
      e.preventDefault();
      expand = 1 - e.deltaY * 0.001;
      //最小値・最大値を指定
      //expand = Math.min(Math.max(0.1, expand), 10);
      document.getElementById("processing").textContent = expand.toString();
      
      //画像拡大・縮小
      camera_expand();
      document.getElementById("processing").textContent = "camera_expanded";
    });
    
    //マウスが移動した際に、移動量を保存しておく.
    document.addEventListener("mousemove", (event) => {
      pre_mouseX = mouseX;
      pre_mouseY = mouseY;
      mouseX = event.pageX;
      mouseY = event.pageY;
      mouse_dif_X = mouseX-pre_mouseX;
      mouse_dif_Y = mouseY-pre_mouseY;
      //クリック中のボタンごとにカメラの移動処理.
      if(LEFTmouseDOWN){
        camera_rotate();
      }
      else if(RIGHTmouseDOWN){
        camera_move();
      }
      else if(CENTORmouseDOWN){
        //camera_expand();
      }
    });
    
    function camera_rotate(){ 
      camera.rotation.y += mouse_dif_X*0.003;
      camera.rotation.x += mouse_dif_Y*0.003;
    }
    function camera_move(){
      camera.position.y += -mouse_dif_Y*0.01;
      camera.position.x += mouse_dif_X*0.01;
    }
    function camera_expand(){
      document.getElementById("processing").textContent = "camera_expand start!!";
      //ページがスクロールしないようにイベントキャンセル.
      //e.preventDefault();.
      let hoge = camera.position.y;
      document.getElementById("processing").textContent = hoge.ToString();
      camera.position.y *= expand;
      camera.position.x *= expand;
      document.getElementById("processing").textContent = "camera_expand end!!";
    }

    // 箱を作成
    let geometry,material,box;
    //makebox();

    // pcd読み込み
    let pcdLoader;
    readPCD();

    renderer.render(scene, camera); 

    // カメラの回転
    tick();


    //======================以降、使用関数たち===================-

    // 箱を作成する関数です
    function makebox() {
      geometry = new THREE.BoxGeometry(0.5, 0.5, 0.5);
      material = new THREE.MeshNormalMaterial();
      box = new THREE.Mesh(geometry, material);
      scene.add(box);
      
      document.getElementById("processing").textContent = 'ボックス作成';
    }


    // pcdを読み込む関数です
    function readPCD() {
      const pcdLoader = new THREE.PCDLoader();
      //document.getElementById("processing").textContent = 'PCDローダー作成';
      pcdLoader.load(
        // pcdファイルの場所
        '../pcd/0000000048.pcd',
        // pcdが読み込まれたときの処理
        function ( points ) {
          scene.add( points );
        }
      );
      document.getElementById("processing").textContent = 'pcd読み込み完了';
    }

    // 毎フレーム時に実行されるループイベントです
    function tick() {
      //camera.rotation.y += 0.01;
      renderer.render(scene, camera); // レンダリング

      requestAnimationFrame(tick);
    }

    //カメラをマウスに追従させる関数.
    function move_camera(){

    }
}
