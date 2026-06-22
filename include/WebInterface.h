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
        .btn-cal {
            grid-column: 1 / -1;
            padding: 10px;
            margin-top: 10px;
            background: linear-gradient(135deg, #10b981 0%, #059669 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            box-shadow: 0 4px 10px rgba(16, 185, 129, 0.3);
        }
        .btn-cal:active { transform: scale(0.95); }
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
                <button class="btn-cal" onclick="calibrate()">📐 Calibrate Level</button>
            </div>
        </div>
        
        <div class="bg-gray-950 p-5 flex justify-between text-center rounded-b-xl border-t border-gray-700">
            <div><p class="text-[10px] text-gray-500 uppercase tracking-widest">Coxa</p><p id="outCx" class="text-lg font-mono font-bold text-blue-400">90&deg;</p></div>
            <div><p class="text-[10px] text-gray-500 uppercase tracking-widest">Femur</p><p id="outFm" class="text-lg font-mono font-bold text-green-400">90&deg;</p></div>
            <div><p class="text-[10px] text-gray-500 uppercase tracking-widest">Tibia</p><p id="outTb" class="text-lg font-mono font-bold text-red-400">90&deg;</p></div>
        </div>
    </div>

    <!-- Floating Joysticks -->
    <div class="absolute bottom-6 left-6 flex flex-col items-center bg-gray-900/80 p-3 rounded-2xl border border-gray-700 shadow-2xl backdrop-blur-md">
        <span class="text-[10px] text-gray-400 mb-2 font-semibold tracking-wider">THROTTLE / YAW</span>
        <canvas id="joyLeft" width="140" height="140" class="bg-gray-950 rounded-full shadow-inner border-2 border-gray-600 touch-none"></canvas>
    </div>

    <div class="absolute bottom-6 right-80 mr-6 flex flex-col items-center bg-gray-900/80 p-3 rounded-2xl border border-gray-700 shadow-2xl backdrop-blur-md">
        <span class="text-[10px] text-gray-400 mb-2 font-semibold tracking-wider">PITCH / ROLL</span>
        <canvas id="joyRight" width="140" height="140" class="bg-gray-950 rounded-full shadow-inner border-2 border-gray-600 touch-none"></canvas>
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
        let rcState = { t: 0, y: 0, p: 0, r: 0, s: 0 };
        let rcInterval;
        let isGaitActive = false;

        function setGait(cmd) {
            if (cmd === 'stop') {
                isGaitActive = false;
                rcState = { t: 0, y: 0, p: 0, r: 0, s: 0 };
                document.getElementById('sliderX').value = 0;
                document.getElementById('sliderY').value = -80;
                document.getElementById('sliderZ').value = 28;
                updateSimulation(); 
            } else {
                isGaitActive = true;
            }
            fetch(`/gait?cmd=${cmd}`);
        }

        function calibrate() {
            fetch('/calibrate');
        }

        function resetRC() {
            setGait('stop');
            if(window.drawLeftJoy) window.drawLeftJoy(0, 0);
            if(window.drawRightJoy) window.drawRightJoy(0, 0);
        }

        function initJoystick(canvasId, isLeft) {
            const canvas = document.getElementById(canvasId);
            const ctx = canvas.getContext('2d');
            const radius = canvas.width / 2;
            const center = { x: radius, y: radius };
            const joyRadius = 25;
            
            let active = false;

            function draw(nx, ny) {
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                ctx.beginPath(); ctx.strokeStyle = '#374151'; ctx.lineWidth = 1;
                ctx.moveTo(center.x, 0); ctx.lineTo(center.x, canvas.height);
                ctx.moveTo(0, center.y); ctx.lineTo(canvas.width, center.y); ctx.stroke();
                
                let px = center.x + nx * (radius - joyRadius);
                let py = center.y - ny * (radius - joyRadius);
                ctx.beginPath(); ctx.arc(px, py, joyRadius, 0, Math.PI * 2);
                ctx.fillStyle = '#3b82f6'; ctx.fill();
                ctx.strokeStyle = '#60a5fa'; ctx.lineWidth = 2; ctx.stroke();
            }

            if(isLeft) window.drawLeftJoy = draw; else window.drawRightJoy = draw;
            draw(0, 0);
            
            function updateState(x, y) {
                let nx = (x - center.x) / (radius - joyRadius);
                let ny = -(y - center.y) / (radius - joyRadius);
                let dist = Math.sqrt(nx*nx + ny*ny);
                if (dist > 1.0) { nx /= dist; ny /= dist; }
                
                if (isLeft) { rcState.y = nx; rcState.t = ny; rcState.s = 0; } 
                else { rcState.r = nx; rcState.p = ny; rcState.s = 0; }
                draw(nx, ny);
            }

            function handleStart(e) { 
                isGaitActive = false; // Break the preset gait macro if active
                active = true; 
                handleMove(e); 
            }
            function handleEnd(e) { active = false; updateState(center.x, center.y); }
            function handleMove(e) {
                if (!active) return;
                e.preventDefault();
                let rect = canvas.getBoundingClientRect();
                let clientX = e.touches ? e.touches[0].clientX : e.clientX;
                let clientY = e.touches ? e.touches[0].clientY : e.clientY;
                updateState(clientX - rect.left, clientY - rect.top);
            }

            canvas.addEventListener('mousedown', handleStart);
            window.addEventListener('mouseup', handleEnd);
            window.addEventListener('mousemove', handleMove);
            canvas.addEventListener('touchstart', handleStart, {passive: false});
            canvas.addEventListener('touchend', handleEnd);
            canvas.addEventListener('touchmove', handleMove, {passive: false});
        }

        function startRCLoop() {
            rcInterval = setInterval(() => {
                if (isGaitActive) return; // Yield completely to backend Gait state
                fetch(`/rc?t=${rcState.t.toFixed(2)}&y=${rcState.y.toFixed(2)}&p=${rcState.p.toFixed(2)}&r=${rcState.r.toFixed(2)}&s=${rcState.s.toFixed(2)}`)
                .then(r => {
                    if (r.ok) {
                        document.getElementById('connDot').className = "w-3 h-3 rounded-full bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.8)]";
                        document.getElementById('connText').innerText = "RC Connected";
                    }
                }).catch(e => {
                    document.getElementById('connDot').className = "w-3 h-3 rounded-full bg-red-500 shadow-[0_0_8px_rgba(239,68,68,0.8)]";
                    document.getElementById('connText').innerText = "Disconnected";
                });
            }, 100); // 10Hz RC Loop
        }

        window.addEventListener('DOMContentLoaded', () => {
            initJoystick('joyLeft', true);
            initJoystick('joyRight', false);
            startRCLoop();
        });

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
