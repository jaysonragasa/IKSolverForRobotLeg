#pragma once

#include <Arduino.h>

// --- Web App Interface (Saved in PROGMEM to save RAM) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>3DOF Dog Leg Simulator</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/controls/OrbitControls.js"></script>
    <style>
        body { margin: 0; overflow: hidden; background-color: #111827; }
        #canvas-container { position: absolute; top: 0; left: 0; width: 100vw; height: 100vh; }
        .panel { position: absolute; top: 20px; right: 20px; max-height: 90vh; overflow-y: auto; }
        input[type=range] { accent-color: #3b82f6; }
    </style>
</head>
<body class="text-white font-sans antialiased selection:bg-blue-500">

    <div id="canvas-container"></div>

    <div class="panel w-80 bg-gray-800/90 backdrop-blur-md rounded-xl shadow-2xl border border-gray-700 flex flex-col">
        <div class="p-5 border-b border-gray-700 flex justify-between items-center">
            <div>
                <h1 class="text-xl font-bold text-blue-400">ESP32 Dog Leg</h1>
                <p class="text-gray-400 text-xs mt-1">Front-Right Controller</p>
            </div>
            <!-- Hardware Connection Status Light -->
            <div class="flex flex-col items-end">
                <div class="flex items-center space-x-2">
                    <span id="connDot" class="w-3 h-3 rounded-full bg-red-500 shadow-[0_0_8px_rgba(239,68,68,0.8)]"></span>
                </div>
                <span id="connText" class="text-[10px] text-gray-400 mt-1 uppercase tracking-wider">Disconnected</span>
            </div>
        </div>
        
        <div class="p-5 space-y-5 flex-1">
            <div class="space-y-3">
                <h2 class="text-sm font-semibold text-gray-300 uppercase tracking-wider">Target Position</h2>
                <div>
                    <div class="flex justify-between text-xs mb-1"><label>X (Forward)</label><span id="valX">0 mm</span></div>
                    <input type="range" id="sliderX" class="w-full" min="-60" max="60" value="0">
                </div>
                <div>
                    <div class="flex justify-between text-xs mb-1"><label>Y (Vertical)</label><span id="valY">-80 mm</span></div>
                    <input type="range" id="sliderY" class="w-full" min="-120" max="-30" value="-80">
                </div>
                <div>
                    <div class="flex justify-between text-xs mb-1"><label>Z (Lateral)</label><span id="valZ">28 mm</span></div>
                    <input type="range" id="sliderZ" class="w-full" min="-20" max="60" value="28">
                </div>
            </div>

            <div class="space-y-3 pt-3 border-t border-gray-700">
                <h2 class="text-sm font-semibold text-gray-300 uppercase tracking-wider">Joint Offsets</h2>
                <div>
                    <div class="flex justify-between text-xs mb-1"><label>Coxa Offset</label><span id="valCX">0&deg;</span></div>
                    <input type="range" id="sliderCX" class="w-full" min="-45" max="45" value="0">
                </div>
                <div>
                    <div class="flex justify-between text-xs mb-1"><label>Femur Offset</label><span id="valFM">0&deg;</span></div>
                    <input type="range" id="sliderFM" class="w-full" min="-45" max="45" value="0">
                </div>
                <div>
                    <div class="flex justify-between text-xs mb-1"><label>Tibia Offset</label><span id="valTB">0&deg;</span></div>
                    <input type="range" id="sliderTB" class="w-full" min="-45" max="45" value="0">
                </div>
            </div>

            <div class="space-y-3 pt-3 border-t border-gray-700">
                <h2 class="text-sm font-semibold text-gray-300 uppercase tracking-wider">Gait Controls</h2>
                <div class="grid grid-cols-3 gap-2 text-center font-semibold text-white">
                    <button onclick="setGait('rotate_left')" class="bg-gray-700 hover:bg-gray-600 rounded py-2 px-1 text-[10px] shadow-md transition-colors">ROT L</button>
                    <button onclick="setGait('forward')" class="bg-blue-600 hover:bg-blue-500 rounded py-2 px-1 text-[10px] shadow-md transition-colors">FWD</button>
                    <button onclick="setGait('rotate_right')" class="bg-gray-700 hover:bg-gray-600 rounded py-2 px-1 text-[10px] shadow-md transition-colors">ROT R</button>
                    
                    <button onclick="setGait('strafe_left')" class="bg-gray-700 hover:bg-gray-600 rounded py-2 px-1 text-[10px] shadow-md transition-colors">STR L</button>
                    <button onclick="setGait('stop')" class="bg-red-600 hover:bg-red-500 rounded py-2 px-1 text-[10px] shadow-md transition-colors">STOP</button>
                    <button onclick="setGait('strafe_right')" class="bg-gray-700 hover:bg-gray-600 rounded py-2 px-1 text-[10px] shadow-md transition-colors">STR R</button>
                    
                    <div></div>
                    <button onclick="setGait('backward')" class="bg-gray-700 hover:bg-gray-600 rounded py-2 px-1 text-[10px] shadow-md transition-colors">BACK</button>
                    <div></div>
                </div>
            </div>
        </div>
        
        <div class="bg-gray-950 p-5 flex justify-between text-center rounded-b-xl border-t border-gray-700">
            <div><p class="text-[10px] text-gray-500 uppercase tracking-widest">Coxa</p><p id="outCx" class="text-lg font-mono font-bold text-blue-400">90&deg;</p></div>
            <div><p class="text-[10px] text-gray-500 uppercase tracking-widest">Femur</p><p id="outFm" class="text-lg font-mono font-bold text-green-400">90&deg;</p></div>
            <div><p class="text-[10px] text-gray-500 uppercase tracking-widest">Tibia</p><p id="outTb" class="text-lg font-mono font-bold text-red-400">90&deg;</p></div>
        </div>
    </div>

    <script>
        // --- THREE.JS SETUP ---
        const container = document.getElementById('canvas-container');
        const scene = new THREE.Scene();
        scene.fog = new THREE.FogExp2(0x111827, 0.003);

        const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 1000);
        camera.position.set(120, 80, 150);

        const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
        renderer.setSize(window.innerWidth, window.innerHeight);
        renderer.setPixelRatio(window.devicePixelRatio);
        container.appendChild(renderer.domElement);

        const controls = new THREE.OrbitControls(camera, renderer.domElement);
        controls.enableDamping = true;
        controls.dampingFactor = 0.05;
        controls.target.set(0, -40, 0);

        scene.add(new THREE.AmbientLight(0xffffff, 0.6));
        const dirLight = new THREE.DirectionalLight(0xffffff, 0.8);
        dirLight.position.set(100, 200, 100);
        scene.add(dirLight);

        const gridHelper = new THREE.GridHelper(300, 30, 0x374151, 0x1f2937);
        gridHelper.position.y = -120;
        scene.add(gridHelper);
        scene.add(new THREE.AxesHelper(50));

        // --- ROBOT LEG MODEL ---
        const L_COXA = 28.0, L_FEMUR = 50.0, L_TIBIA = 72.0;
        const matCoxa = new THREE.MeshPhongMaterial({ color: 0x3b82f6, flatShading: true });
        const matFemur = new THREE.MeshPhongMaterial({ color: 0x10b981, flatShading: true });
        const matTibia = new THREE.MeshPhongMaterial({ color: 0xef4444, flatShading: true });
        const matJoint = new THREE.MeshPhongMaterial({ color: 0x9ca3af });

        const baseMesh = new THREE.Mesh(new THREE.BoxGeometry(20, 20, 40), new THREE.MeshPhongMaterial({ color: 0x4b5563 }));
        baseMesh.position.set(-10, 0, 0); scene.add(baseMesh);

        const coxaGroup = new THREE.Group(); scene.add(coxaGroup);
        const coxaGeo = new THREE.CylinderGeometry(4, 4, L_COXA, 16); coxaGeo.rotateX(Math.PI / 2); coxaGeo.translate(0, 0, L_COXA / 2);
        coxaGroup.add(new THREE.Mesh(coxaGeo, matCoxa)); coxaGroup.add(new THREE.Mesh(new THREE.SphereGeometry(5.5), matJoint));

        const femurGroup = new THREE.Group(); femurGroup.position.set(0, 0, L_COXA); coxaGroup.add(femurGroup);
        const femurGeo = new THREE.CylinderGeometry(3.5, 3.5, L_FEMUR, 16); femurGeo.translate(0, -L_FEMUR / 2, 0);
        femurGroup.add(new THREE.Mesh(femurGeo, matFemur)); femurGroup.add(new THREE.Mesh(new THREE.SphereGeometry(4.5), matJoint));

        const tibiaGroup = new THREE.Group(); tibiaGroup.position.set(0, -L_FEMUR, 0); femurGroup.add(tibiaGroup);
        const tibiaGeo = new THREE.CylinderGeometry(2.5, 1.5, L_TIBIA, 16); tibiaGeo.translate(0, -L_TIBIA / 2, 0);
        tibiaGroup.add(new THREE.Mesh(tibiaGeo, matTibia)); tibiaGroup.add(new THREE.Mesh(new THREE.SphereGeometry(3.5), matJoint));

        const targetMarker = new THREE.Mesh(new THREE.SphereGeometry(4, 16, 16), new THREE.MeshBasicMaterial({ color: 0xff00ff, wireframe: true }));
        scene.add(targetMarker);

        // --- IK SOLVER (Browser Side for UI) ---
        function solveDogIK(x, y, z, offC, offF, offT) {
            let L_yz = Math.sqrt(y * y + z * z);
            if (L_yz < L_COXA) L_yz = L_COXA;
            let L_drop = Math.sqrt(L_yz * L_yz - L_COXA * L_COXA);
            let alpha = Math.atan2(z, Math.abs(y));
            let beta = Math.asin(L_COXA / L_yz);
            let theta_c = alpha - beta;
            let D = Math.sqrt(x * x + L_drop * L_drop);
            if (D > (L_FEMUR + L_TIBIA)) D = L_FEMUR + L_TIBIA;
            if (D < 0.1) D = 0.1;
            let angle_to_target = Math.atan2(x, L_drop);
            let cos_femur = Math.max(-1.0, Math.min(1.0, (L_FEMUR * L_FEMUR + D * D - L_TIBIA * L_TIBIA) / (2.0 * L_FEMUR * D)));
            let cos_tibia = Math.max(-1.0, Math.min(1.0, (L_FEMUR * L_FEMUR + L_TIBIA * L_TIBIA - D * D) / (2.0 * L_FEMUR * L_TIBIA)));
            let inner_femur = Math.acos(cos_femur);
            let inner_tibia = Math.acos(cos_tibia);
            let knee_bend = Math.PI - inner_tibia;

            let idealC = Math.max(0, Math.min(180, 90.0 + (theta_c * 180 / Math.PI) + offC));
            let idealF = Math.max(0, Math.min(180, 90.0 + ((angle_to_target + inner_femur) * 180 / Math.PI) + offF));
            let idealT = Math.max(0, Math.min(180, (inner_tibia * 180 / Math.PI) + offT));

            const SCALE = 147.0 / 180.0;
            return {
                servo: { c: idealC * SCALE, f: idealF * SCALE, t: idealT * SCALE },
                rad: {
                    c: (idealC - 90.0) * (Math.PI / 180.0),
                    f: (idealF - 90.0) * (Math.PI / 180.0),
                    t: Math.PI - (idealT * (Math.PI / 180.0))
                }
            };
        }

        // --- HARDWARE COMMS ---
        let sendTimeout;

        function setGait(cmd) {
            if (cmd === 'stop') {
                document.getElementById('sliderX').value = 0;
                document.getElementById('sliderY').value = -80;
                document.getElementById('sliderZ').value = 28;
                document.getElementById('sliderCX').value = 0;
                document.getElementById('sliderFM').value = 0;
                document.getElementById('sliderTB').value = 0;
                updateSimulation(); // This will sync the true default stance to the ESP32
            }
            fetch(`/gait?cmd=${cmd}`)
            .then(response => {
                if (response.ok) {
                    document.getElementById('connDot').className = "w-3 h-3 rounded-full bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.8)]";
                    document.getElementById('connText').innerText = `Gait: ${cmd.toUpperCase()}`;
                }
            })
            .catch(e => {
                console.log('Error setting gait');
                document.getElementById('connDot').className = "w-3 h-3 rounded-full bg-red-500 shadow-[0_0_8px_rgba(239,68,68,0.8)]";
                document.getElementById('connText').innerText = "Disconnected";
            });
        }

        function sendToESP32(tx, ty, tz, oc, of, ot) {
            clearTimeout(sendTimeout);
            sendTimeout = setTimeout(() => {
                fetch(`/ik?x=${tx}&y=${ty}&z=${tz}&cx=${oc}&fm=${of}&tb=${ot}`)
                .then(response => {
                    if (response.ok) {
                        document.getElementById('connDot').className = "w-3 h-3 rounded-full bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.8)]";
                        document.getElementById('connText').innerText = "Hardware Synced";
                    } else {
                        throw new Error('Bad Network Response');
                    }
                })
                .catch(e => {
                    console.log('ESP32 busy or disconnected');
                    document.getElementById('connDot').className = "w-3 h-3 rounded-full bg-red-500 shadow-[0_0_8px_rgba(239,68,68,0.8)]";
                    document.getElementById('connText').innerText = "Disconnected";
                });
            }, 50); // Throttled to 20 updates per second
        }

        function updateSimulation() {
            const tx = parseFloat(document.getElementById('sliderX').value);
            const ty = parseFloat(document.getElementById('sliderY').value);
            const tz = parseFloat(document.getElementById('sliderZ').value);
            const oc = parseFloat(document.getElementById('sliderCX').value);
            const of = parseFloat(document.getElementById('sliderFM').value);
            const ot = parseFloat(document.getElementById('sliderTB').value);

            document.getElementById('valX').innerText = tx + ' mm';
            document.getElementById('valY').innerText = ty + ' mm';
            document.getElementById('valZ').innerText = tz + ' mm';
            document.getElementById('valCX').innerText = oc + '\xB0';
            document.getElementById('valFM').innerText = of + '\xB0';
            document.getElementById('valTB').innerText = ot + '\xB0';

            targetMarker.position.set(tx, ty, tz);
            const ik = solveDogIK(tx, ty, tz, oc, of, ot);

            coxaGroup.rotation.x = -ik.rad.c; 
            femurGroup.rotation.z = ik.rad.f; 
            tibiaGroup.rotation.z = -ik.rad.t; 

            document.getElementById('outCx').innerText = ik.servo.c.toFixed(1) + '\xB0';
            document.getElementById('outFm').innerText = ik.servo.f.toFixed(1) + '\xB0';
            document.getElementById('outTb').innerText = ik.servo.t.toFixed(1) + '\xB0';

            // Push to ESP32 Hardware
            sendToESP32(tx, ty, tz, oc, of, ot);
        }

        document.querySelectorAll('input[type="range"]').forEach(slider => {
            slider.addEventListener('input', updateSimulation);
        });

        window.addEventListener('resize', () => {
            camera.aspect = window.innerWidth / window.innerHeight;
            camera.updateProjectionMatrix();
            renderer.setSize(window.innerWidth, window.innerHeight);
        });

        window.onload = function () {
            updateSimulation();
            function animate() {
                requestAnimationFrame(animate);
                controls.update();
                renderer.render(scene, camera);
            }
            animate();
        };
    </script>
</body>
</html>
)rawliteral";
