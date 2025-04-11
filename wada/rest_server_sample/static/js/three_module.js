			import * as THREE from 'three';
			import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
			import { PCDLoader } from 'three/addons/loaders/PCDLoader.js';

			let camera, scene, renderer;

		    scene_init();
			render();
            //tick();

			function scene_init() {
                // サイズを指定
                const width = 960;
                const height = 540;
            
                // レンダラーを作成
                renderer = new THREE.WebGLRenderer({
                  canvas: document.querySelector("#myCanvas"),
                });
                renderer.setPixelRatio(window.devicePixelRatio);
                renderer.setSize(width, height);
            
                document.getElementById("processing").textContent = 'レンダラー作成';
            
                // シーンを作成
                scene = new THREE.Scene();
                document.getElementById("processing").textContent = 'シーン作成';
            
                // カメラを作成
                camera = new THREE.PerspectiveCamera(45, width / height);
                camera.position.set(0, 0, 1);
                document.getElementById("processing").textContent = 'カメラ作成';

                // カメラ回転用コントローラーの作成
				const controls = new OrbitControls( camera, renderer.domElement );
				controls.addEventListener( 'change', render ); // use if there is no animation loop
				controls.minDistance = 0.5;
				controls.maxDistance = 10;

                // pcd読み込み
				const pcdloader = new PCDLoader();
				pcdloader.load( 
                    //pcdファイルの場所
                    '../pcd/0000000048.pcd',
                    //pcdが読み込まれたときの処理
                    function ( points ) {
					points.geometry.center();
					points.geometry.rotateX( Math.PI );
					points.name = '0000000048.pcd';
					scene.add( points );
					render();
				    }
                );
			}

			function render() {

				renderer.render( scene, camera );

			}

            function tick() {
              camera.rotation.y += 0.01;
              renderer.render(scene, camera); // レンダリング
        
              document.getElementById("processing").textContent = 'フレーム処理';
        
              requestAnimationFrame(tick);
            }
