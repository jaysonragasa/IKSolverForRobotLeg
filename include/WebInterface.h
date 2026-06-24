#pragma once

#include <Arduino.h>

// --- Web App Interface (Saved in PROGMEM to save RAM) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Robot Controller</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/controls/OrbitControls.js"></script>
    <style>
        body { margin: 0; overflow: hidden; background-color: #111827; touch-action: none; font-family: sans-serif; }
        #canvas-container { position: absolute; top: 0; left: 0; width: 100vw; height: 100vh; z-index: 1; }
        
        /* UI Layer */
        .ui-layer { position: absolute; z-index: 10; pointer-events: none; }
        .ui-layer > * { pointer-events: auto; }
        
        .panel { background: rgba(31, 41, 55, 0.85); backdrop-filter: blur(12px); border: 1px solid rgba(75, 85, 99, 0.4); border-radius: 1rem; box-shadow: 0 25px 50px -12px rgba(0,0,0,0.5); }
        
        input[type=range] { accent-color: #3b82f6; width: 100%; }
        
        .btn-gait { background: rgba(55, 65, 81, 0.9); border: 1px solid #4b5563; border-radius: 0.5rem; color: #d1d5db; font-size: 11px; font-weight: bold; width: 50px; height: 50px; display: flex; align-items: center; justify-content: center; box-shadow: 0 4px 6px rgba(0,0,0,0.3); transition: all 0.1s; }
        .btn-gait:active { transform: scale(0.95); background: #3b82f6; color: white; border-color: #60a5fa; }
        
        .modal-overlay { position: absolute; inset: 0; background: rgba(0,0,0,0.7); backdrop-filter: blur(4px); z-index: 50; display: none; align-items: center; justify-content: center; pointer-events: auto; }
        .modal { background: #1f2937; width: 90%; max-width: 800px; max-height: 85vh; overflow-y: auto; border-radius: 1.5rem; padding: 1.5rem; border: 1px solid #374151; box-shadow: 0 25px 50px -12px rgba(0,0,0,1); }
        
        .toggle-btn { background: #374151; color: #9ca3af; border: 1px solid #4b5563; padding: 0.5rem 1rem; border-radius: 0.5rem; font-weight: bold; font-size: 0.75rem; transition: all 0.2s; }
        .toggle-btn.active { background: #3b82f6; color: white; border-color: #60a5fa; box-shadow: 0 0 15px rgba(59,130,246,0.5); }

        /* Custom Scrollbar */
        ::-webkit-scrollbar { width: 6px; }
        ::-webkit-scrollbar-track { background: transparent; }
        ::-webkit-scrollbar-thumb { background: #4b5563; border-radius: 3px; }
        ::-webkit-scrollbar-thumb:hover { background: #6b7280; }
        * { scrollbar-width: thin; scrollbar-color: #4b5563 transparent; }
    </style>
</head>
<body class="text-white antialiased">
    <div id="canvas-container"></div>

    <!-- Top Header -->
    <div class="ui-layer top-4 left-4 panel p-3 flex items-center space-x-3">
        <div>
            <h1 class="text-sm font-bold text-blue-400">ASKALBOT RC</h1>
            <div class="flex items-center space-x-1 mt-0.5">
                <span id="connDot" class="w-2 h-2 rounded-full bg-red-500 shadow-[0_0_8px_rgba(239,68,68,0.8)]"></span>
                <span id="connText" class="text-[9px] text-gray-400 uppercase tracking-wider">Disconnected</span>
            </div>
        </div>
    </div>

    <!-- Top Right Actions -->
    <!-- Top Left: Poses (Below Status) -->
    <div class="ui-layer top-28 left-4 flex flex-col space-y-3">
        <button onclick="setPose(-26, -31, 38, 0, 0)" class="panel px-4 py-2 text-xs font-bold text-gray-300 hover:text-white transition active:scale-95 shadow-[0_0_10px_rgba(0,0,0,0.5)]">🛌 Lie Down</button>
        <button onclick="setPose(0, -70, 28, -35, 0)" class="panel px-4 py-2 text-xs font-bold text-gray-300 hover:text-white transition active:scale-95 shadow-[0_0_10px_rgba(0,0,0,0.5)]">🪑 Sit</button>
        <button onclick="wave()" class="panel px-4 py-2 text-xs font-bold text-gray-300 hover:text-white transition active:scale-95 shadow-[0_0_10px_rgba(0,0,0,0.5)]">🐾 Wave</button>
        <button onclick="setPose(0, -100, 28, 0, 0)" class="panel px-4 py-2 text-xs font-bold text-gray-300 hover:text-white transition active:scale-95 shadow-[0_0_10px_rgba(0,0,0,0.5)]">🐕 Stand Up</button>
    </div>

    <!-- Top Right: Config -->
    <div class="ui-layer top-4 right-4 flex space-x-2">
        <button onclick="openModal('setup')" class="panel px-4 py-3 text-xs font-bold text-gray-300 hover:text-white transition active:scale-95">⚙️ Setup</button>
        <button onclick="openModal('tune')" class="panel px-4 py-3 text-xs font-bold text-gray-300 hover:text-white transition active:scale-95">🎛️ Tuning</button>
    </div>

    <!-- Bottom Left: Joystick -->
    <div id="joyLeftContainer" class="ui-layer bottom-6 left-6 flex flex-col items-center">
        <span class="text-[10px] text-gray-400 mb-2 font-bold tracking-wider drop-shadow-md">THROTTLE / YAW</span>
        <canvas id="joyLeft" width="250" height="250" class="panel rounded-full touch-none"></canvas>
    </div>

    <!-- Bottom Right: Joystick -->
    <div id="joyRightContainer" class="ui-layer bottom-6 right-6 flex flex-col items-center">
        <span class="text-[10px] text-gray-400 mb-2 font-bold tracking-wider drop-shadow-md">PITCH / ROLL</span>
        <canvas id="joyRight" width="250" height="250" class="panel rounded-full touch-none"></canvas>
    </div>

    <!-- Bottom Center: Gait & AutoBalance -->
    <div class="ui-layer bottom-6 left-1/2 transform -translate-x-1/2 flex flex-col items-center">
        <div class="grid grid-cols-3 gap-2 mb-4">
            <button onclick="setGait('rotate_left')" class="btn-gait">ROT L</button>
            <button onclick="setGait('forward')" class="btn-gait">FWD</button>
            <button onclick="setGait('rotate_right')" class="btn-gait">ROT R</button>
            
            <button onclick="setGait('strafe_left')" class="btn-gait">STR L</button>
            <button onclick="setGait('stop')" class="btn-gait !bg-red-900 !border-red-700 !text-red-300">STOP</button>
            <button onclick="setGait('strafe_right')" class="btn-gait">STR R</button>
            
            <button onclick="toggleJoy('L')" class="btn-gait !bg-gray-800 !text-gray-400">JS-L</button>
            <button onclick="setGait('backward')" class="btn-gait">BACK</button>
            <button onclick="toggleJoy('R')" class="btn-gait !bg-gray-800 !text-gray-400">JS-R</button>
        </div>
        
        <div class="flex space-x-2 w-full">
            <button id="btnAB" onclick="toggleAB()" class="toggle-btn flex-1">Auto-Balance: OFF</button>
            <button onclick="calibrate()" class="toggle-btn flex-1 !bg-emerald-600 hover:!bg-emerald-500 !text-white !border-emerald-500 shadow-[0_0_10px_rgba(16,185,129,0.3)]">📐 Calibrate</button>
        </div>
    </div>

    <!-- MODALS -->
    <div id="modalOverlay" class="modal-overlay">
        <!-- SETUP MODAL -->
        <div id="modal-setup" class="modal hidden">
            <div class="flex justify-between items-center mb-6">
                <h2 class="text-lg font-bold text-white">⚙️ Setup & Calibration</h2>
                <button onclick="closeModal()" class="text-gray-400 hover:text-white text-xl font-bold">&times;</button>
            </div>
            
            <div class="space-y-6">
                <!-- Target Pose IK -->
                <div class="bg-gray-800 p-4 rounded-xl border border-gray-700">
                    <h3 class="text-xs font-bold text-gray-400 uppercase tracking-wider mb-3">Target Pose (IK)</h3>
                    <div class="space-y-3">
                        <div><div class="flex justify-between text-xs mb-1"><label>X (Forward)</label><span id="valX">0 mm</span></div><input type="range" id="sliderX" min="-60" max="60" value="0"></div>
                        <div><div class="flex justify-between text-xs mb-1"><label>Y (Vertical)</label><span id="valY">-100 mm</span></div><input type="range" id="sliderY" min="-120" max="-30" value="-100"></div>
                        <div><div class="flex justify-between text-xs mb-1"><label>Z (Lateral)</label><span id="valZ">28 mm</span></div><input type="range" id="sliderZ" min="-20" max="60" value="28"></div>
                        <div><div class="flex justify-between text-xs mb-1"><label>Pitch</label><span id="valPitch">0 &deg;</span></div><input type="range" id="sliderPitch" min="-45" max="45" value="0"></div>
                        <div><div class="flex justify-between text-xs mb-1"><label>Roll</label><span id="valRoll">0 &deg;</span></div><input type="range" id="sliderRoll" min="-30" max="30" value="0"></div>
                    </div>
                </div>

                <!-- Servo Offsets -->
                <div class="bg-gray-800 p-4 rounded-xl border border-gray-700">
                    <h3 class="text-xs font-bold text-gray-400 uppercase tracking-wider mb-3">Servo Offsets (Ch 0-15)</h3>
                    <div id="servo-grid">
                        <!-- JS injected -->
                    </div>
                </div>
            </div>
        </div>

        <!-- TUNING MODAL -->
        <div id="modal-tune" class="modal hidden">
            <div class="flex justify-between items-center mb-6">
                <h2 class="text-lg font-bold text-white">🎛️ Suspension Tuning</h2>
                <button onclick="closeModal()" class="text-gray-400 hover:text-white text-xl font-bold">&times;</button>
            </div>

            <div class="space-y-6">
                <!-- PID Toggles & Sliders -->
                <div class="bg-gray-800 p-4 rounded-xl border border-gray-700">
                    <div class="flex justify-between items-center mb-4">
                        <h3 class="text-xs font-bold text-gray-400 uppercase tracking-wider">Advanced PID</h3>
                        <button id="btnPID" onclick="togglePID()" class="toggle-btn">PID: OFF</button>
                    </div>
                    
                    <div class="space-y-4">
                        <div><div class="flex justify-between text-xs mb-1"><label>Kp (Stiffness)</label><span id="valKp">1.00</span></div><input type="range" id="sliderKp" class="pid-slider" min="0" max="5" step="0.1" value="1.0"></div>
                        <div><div class="flex justify-between text-xs mb-1"><label>Ki (Sag Correction)</label><span id="valKi">0.00</span></div><input type="range" id="sliderKi" class="pid-slider" min="0" max="2" step="0.05" value="0.0"></div>
                        <div><div class="flex justify-between text-xs mb-1"><label>Kd (Dampening)</label><span id="valKd">0.00</span></div><input type="range" id="sliderKd" class="pid-slider" min="0" max="2" step="0.05" value="0.0"></div>
                    </div>
                </div>

                <!-- Deadband Slider -->
                <div class="bg-gray-800 p-4 rounded-xl border border-gray-700">
                    <h3 class="text-xs font-bold text-gray-400 uppercase tracking-wider mb-4">IMU Deadband</h3>
                    <div><div class="flex justify-between text-xs mb-1"><label>Threshold (deg)</label><span id="valDB">0.0&deg;</span></div><input type="range" id="sliderDB" min="0" max="10" step="0.5" value="0.0"></div>
                </div>
            </div>
        </div>
    </div>

    <script>
        // --- STATE ---
        let stateAB = false;
        let statePID = false;
        
        function openModal(id) {
            document.getElementById('modalOverlay').style.display = 'flex';
            document.getElementById('modal-setup').classList.add('hidden');
            document.getElementById('modal-tune').classList.add('hidden');
            document.getElementById('modal-' + id).classList.remove('hidden');
        }
        function closeModal() {
            document.getElementById('modalOverlay').style.display = 'none';
        }

        function setPose(x, y, z, p, r) {
            fetch('/anim?mode=0');
            fetch(`/pose?x=${x}&y=${y}&z=${z}&p=${p}&r=${r}`);
        }

        function wave() {
            fetch('/anim?mode=1');
        }

        function syncToggles() {
            fetch(`/toggle?ab=${stateAB?1:0}&pid=${statePID?1:0}`);
            
            const btnAB = document.getElementById('btnAB');
            if (stateAB) { btnAB.classList.add('active'); btnAB.innerText = 'Auto-Balance: ON'; }
            else { btnAB.classList.remove('active'); btnAB.innerText = 'Auto-Balance: OFF'; }

            const btnPID = document.getElementById('btnPID');
            if (statePID) { btnPID.classList.add('active'); btnPID.innerText = 'PID: ON'; }
            else { btnPID.classList.remove('active'); btnPID.innerText = 'PID: OFF'; }
        }

        function toggleAB() { stateAB = !stateAB; syncToggles(); }
        function togglePID() { statePID = !statePID; syncToggles(); }

        // --- SERVO SLIDERS GENERATION ---
        const servoGrid = document.getElementById('servo-grid');
        let html = '';
        const jointNames = ["Coxa", "Femur", "Tibia"];
        
        for (let g = 0; g < 4; g++) {
            html += `<div class="bg-gray-700/50 p-3 rounded-lg mb-3">`;
            html += `<h4 class="text-[10px] font-bold text-gray-400 mb-2 tracking-wider">LEG ${g}</h4>`;
            html += `<div class="grid grid-cols-3 gap-3">`;
            for (let j = 0; j < 3; j++) {
                html += `
                    <div class="flex flex-col items-center w-full">
                        <div class="text-[10px] text-gray-400 font-bold mb-1">${jointNames[j]} <span id="valOff_${g}_${j}" class="text-blue-400 ml-1">0&deg;</span></div>
                        <div class="flex items-center space-x-1 w-full">
                            <button class="bg-gray-600 text-white rounded px-2 py-0.5 text-xs hover:bg-gray-500 transition-colors" onclick="stepOffset(${g}, ${j}, -1)">&lt;</button>
                            <input type="range" id="sliderOff_${g}_${j}" class="off-slider w-full" data-leg="${g}" data-joint="${j}" min="-45" max="45" value="0">
                            <button class="bg-gray-600 text-white rounded px-2 py-0.5 text-xs hover:bg-gray-500 transition-colors" onclick="stepOffset(${g}, ${j}, 1)">&gt;</button>
                        </div>
                    </div>
                `;
            }
            html += `</div></div>`;
        }
        servoGrid.innerHTML = html;

        function stepOffset(leg, joint, dir) {
            let s = document.getElementById(`sliderOff_${leg}_${joint}`);
            let val = parseInt(s.value) + dir;
            if(val < -45) val = -45;
            if(val > 45) val = 45;
            s.value = val;
            document.getElementById(`valOff_${leg}_${joint}`).innerText = val + '\xB0';
            fetch(`/offset?leg=${leg}&joint=${joint}&val=${val}`);
        }

        document.querySelectorAll('.off-slider').forEach(s => {
            s.addEventListener('change', (e) => { // Send on release
                const leg = e.target.getAttribute('data-leg');
                const joint = e.target.getAttribute('data-joint');
                const val = e.target.value;
                document.getElementById(`valOff_${leg}_${joint}`).innerText = val + '\xB0';
                fetch(`/offset?leg=${leg}&joint=${joint}&val=${val}`);
            });
            s.addEventListener('input', (e) => {
                const leg = e.target.getAttribute('data-leg');
                const joint = e.target.getAttribute('data-joint');
                document.getElementById(`valOff_${leg}_${joint}`).innerText = e.target.value + '\xB0';
            });
        });

        // --- FETCH STATE ON LOAD ---
        fetch('/state')
            .then(response => response.json())
            .then(data => {
                // Offsets
                if (data.offsets) {
                    for (let leg = 0; leg < 4; leg++) {
                        for (let joint = 0; joint < 3; joint++) {
                            let val = data.offsets[leg][joint];
                            let s = document.getElementById(`sliderOff_${leg}_${joint}`);
                            if (s) {
                                s.value = val;
                                document.getElementById(`valOff_${leg}_${joint}`).innerText = val + '\xB0';
                            }
                        }
                    }
                }
                
                // IK Pose
                if (data.ik) {
                    let axes = ['X', 'Y', 'Z', 'Pitch', 'Roll'];
                    let units = [' mm', ' mm', ' mm', '\xB0', '\xB0'];
                    for (let i = 0; i < 5; i++) {
                        let s = document.getElementById('slider' + axes[i]);
                        if (s) {
                            s.value = data.ik[i];
                            document.getElementById('val' + axes[i]).innerText = data.ik[i] + units[i];
                        }
                    }
                }

                // Toggles
                if (data.ab !== undefined) { stateAB = data.ab; }
                if (data.pid_en !== undefined) { statePID = data.pid_en; }
                syncToggles();

                // PID
                if (data.pid) {
                    let pidKeys = ['Kp', 'Ki', 'Kd'];
                    for (let i = 0; i < 3; i++) {
                        let s = document.getElementById('slider' + pidKeys[i]);
                        if (s) {
                            s.value = data.pid[i];
                            document.getElementById('val' + pidKeys[i]).innerText = data.pid[i];
                        }
                    }
                }

                // Deadband
                if (data.db !== undefined) {
                    let s = document.getElementById('sliderDB');
                    if (s) {
                        s.value = data.db;
                        document.getElementById('valDB').innerHTML = data.db + '&deg;';
                    }
                }
            })
            .catch(e => console.log('Error fetching state:', e));

        // --- THREE.JS SETUP ---
        const container = document.getElementById('canvas-container');
        const scene = new THREE.Scene();
        scene.fog = new THREE.FogExp2(0x111827, 0.003);

        const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 1000);
        camera.position.set(120, 80, 150);

        const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
        renderer.setSize(window.innerWidth, window.innerHeight);
        container.appendChild(renderer.domElement);

        const controls = new THREE.OrbitControls(camera, renderer.domElement);
        controls.enableDamping = true;
        controls.target.set(0, -40, 0);

        scene.add(new THREE.AmbientLight(0xffffff, 0.6));
        const dirLight = new THREE.DirectionalLight(0xffffff, 0.8);
        dirLight.position.set(100, 200, 100);
        scene.add(dirLight);

        const gridHelper = new THREE.GridHelper(300, 30, 0x374151, 0x1f2937);
        gridHelper.position.y = -120;
        scene.add(gridHelper);
        scene.add(new THREE.AxesHelper(50));

        const L_COXA = 28.0, L_FEMUR = 50.0, L_TIBIA = 72.0;
        const matCoxa = new THREE.MeshPhongMaterial({ color: 0x3b82f6 });
        const matFemur = new THREE.MeshPhongMaterial({ color: 0x10b981 });
        const matTibia = new THREE.MeshPhongMaterial({ color: 0xef4444 });
        
        const coxaGroup = new THREE.Group(); scene.add(coxaGroup);
        const femurGroup = new THREE.Group(); femurGroup.position.set(0, 0, L_COXA); coxaGroup.add(femurGroup);
        const tibiaGroup = new THREE.Group(); tibiaGroup.position.set(0, -L_FEMUR, 0); femurGroup.add(tibiaGroup);
        
        coxaGroup.add(new THREE.Mesh(new THREE.CylinderGeometry(4, 4, L_COXA, 16).rotateX(Math.PI/2).translate(0,0,L_COXA/2), matCoxa));
        femurGroup.add(new THREE.Mesh(new THREE.CylinderGeometry(3.5, 3.5, L_FEMUR, 16).translate(0,-L_FEMUR/2,0), matFemur));
        tibiaGroup.add(new THREE.Mesh(new THREE.CylinderGeometry(2.5, 1.5, L_TIBIA, 16).translate(0,-L_TIBIA/2,0), matTibia));

        const targetMarker = new THREE.Mesh(new THREE.SphereGeometry(4, 16, 16), new THREE.MeshBasicMaterial({ color: 0xff00ff, wireframe: true }));
        scene.add(targetMarker);

        // --- IK SOLVER ---
        function solveDogIK(x, y, z) {
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

            let idealC = Math.max(0, Math.min(180, 90.0 + (theta_c * 180 / Math.PI)));
            let idealF = Math.max(0, Math.min(180, 90.0 + ((angle_to_target + inner_femur) * 180 / Math.PI)));
            let idealT = Math.max(0, Math.min(180, (inner_tibia * 180 / Math.PI)));

            return {
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
        let isGaitActive = false;

        function setGait(cmd) {
            fetch('/anim?mode=0'); // Clear any animation
            if (cmd === 'stop') {
                isGaitActive = false;
                rcState = { t: 0, y: 0, p: 0, r: 0, s: 0 };
                document.getElementById('sliderX').value = 0;
                document.getElementById('sliderY').value = -100;
                document.getElementById('sliderZ').value = 28;
                updateSimulation(); 
            } else {
                isGaitActive = true;
            }
            fetch(`/gait?cmd=${cmd}`);
        }

        function calibrate() { fetch('/calibrate'); }

        function toggleJoy(side) {
            if (side === 'L') {
                const el = document.getElementById('joyLeftContainer');
                el.style.display = (el.style.display === 'none') ? 'flex' : 'none';
            } else {
                const el = document.getElementById('joyRightContainer');
                el.style.display = (el.style.display === 'none') ? 'flex' : 'none';
            }
        }

        function initJoystick(canvasId, isLeft) {
            const canvas = document.getElementById(canvasId);
            const ctx = canvas.getContext('2d');
            const radius = canvas.width / 2;
            const center = { x: radius, y: radius };
            const joyRadius = 22;
            let active = false;

            function draw(nx, ny) {
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                
                // Crosshair
                ctx.beginPath(); 
                ctx.strokeStyle = '#4b5563'; 
                ctx.lineWidth = 1; 
                ctx.moveTo(center.x, 0); ctx.lineTo(center.x, canvas.height);
                ctx.moveTo(0, center.y); ctx.lineTo(canvas.width, center.y); 
                ctx.stroke();
                
                // Outer circle edge
                ctx.beginPath();
                ctx.arc(center.x, center.y, radius - 2, 0, Math.PI * 2);
                ctx.strokeStyle = '#374151';
                ctx.lineWidth = 2;
                ctx.stroke();
                
                // Thumbpad
                let px = center.x + nx * (radius - joyRadius - 5);
                let py = center.y - ny * (radius - joyRadius - 5);
                ctx.beginPath(); 
                ctx.arc(px, py, joyRadius, 0, Math.PI * 2);
                ctx.fillStyle = '#3b82f6'; 
                ctx.fill();
                ctx.lineWidth = 2;
                ctx.strokeStyle = '#93c5fd';
                ctx.stroke();
            }

            if(isLeft) window.drawLeftJoy = draw; else window.drawRightJoy = draw;
            draw(0, 0);
            
            function updateState(x, y) {
                let maxDist = radius - joyRadius - 5;
                let dx = x - center.x;
                let dy = -(y - center.y);
                let nx = dx / maxDist;
                let ny = dy / maxDist;
                let dist = Math.sqrt(nx*nx + ny*ny);
                if (dist > 1.0) { nx /= dist; ny /= dist; }
                
                if (isLeft) { rcState.y = nx; rcState.t = ny; } 
                else { rcState.r = nx; rcState.p = ny; }
                draw(nx, ny);
            }

            let touchId = null;

            function handleStart(e) { 
                isGaitActive = false; 
                active = true; 
                if (e.changedTouches) {
                    touchId = e.changedTouches[0].identifier;
                    let rect = canvas.getBoundingClientRect();
                    updateState(e.changedTouches[0].clientX - rect.left, e.changedTouches[0].clientY - rect.top);
                } else {
                    let rect = canvas.getBoundingClientRect();
                    updateState(e.clientX - rect.left, e.clientY - rect.top);
                }
            }
            function handleEnd(e) { 
                if (e.changedTouches) {
                    let found = false;
                    for (let i = 0; i < e.changedTouches.length; i++) {
                        if (e.changedTouches[i].identifier === touchId) found = true;
                    }
                    if (!found) return;
                }
                active = false; 
                touchId = null; 
                updateState(center.x, center.y); 
            }
            function handleMove(e) {
                if (!active) return;
                e.preventDefault();
                let rect = canvas.getBoundingClientRect();
                let clientX = e.clientX;
                let clientY = e.clientY;
                if (e.changedTouches) {
                    let found = false;
                    for (let i = 0; i < e.changedTouches.length; i++) {
                        if (e.changedTouches[i].identifier === touchId) {
                            clientX = e.changedTouches[i].clientX;
                            clientY = e.changedTouches[i].clientY;
                            found = true;
                            break;
                        }
                    }
                    if (!found) return; 
                }
                updateState(clientX - rect.left, clientY - rect.top);
            }

            canvas.addEventListener('mousedown', handleStart);
            window.addEventListener('mouseup', handleEnd);
            window.addEventListener('mousemove', handleMove, {passive: false});
            canvas.addEventListener('touchstart', handleStart, {passive: false});
            window.addEventListener('touchend', handleEnd);
            window.addEventListener('touchmove', handleMove, {passive: false});
        }

        let lastRcState = { t: 0, y: 0, p: 0, r: 0, s: 0 };
        let lastPingTime = 0;

        setInterval(() => {
            if (isGaitActive) return; 

            let changed = (
                rcState.t !== lastRcState.t ||
                rcState.y !== lastRcState.y ||
                rcState.p !== lastRcState.p ||
                rcState.r !== lastRcState.r ||
                rcState.s !== lastRcState.s
            );

            let now = Date.now();
            
            // Only spam the ESP32 if the joystick actually moved. 
            // Otherwise, just send a heartbeat every 2 seconds to keep connection UI green.
            if (changed || now - lastPingTime > 2000) {
                lastPingTime = now;
                lastRcState = { ...rcState };
                
                fetch(`/rc?t=${rcState.t.toFixed(2)}&y=${rcState.y.toFixed(2)}&p=${rcState.p.toFixed(2)}&r=${rcState.r.toFixed(2)}&s=${rcState.s.toFixed(2)}`)
                .then(r => {
                    if (r.ok) {
                        document.getElementById('connDot').className = "w-2 h-2 rounded-full bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.8)]";
                        document.getElementById('connText').innerText = "RC Connected";
                    }
                }).catch(e => {
                    document.getElementById('connDot').className = "w-2 h-2 rounded-full bg-red-500 shadow-[0_0_8px_rgba(239,68,68,0.8)]";
                    document.getElementById('connText').innerText = "Disconnected";
                });
            }
        }, 100);

        function sendToESP32(tx, ty, tz, tp, tr) {
            clearTimeout(sendTimeout);
            sendTimeout = setTimeout(() => {
                fetch(`/ik?x=${tx}&y=${ty}&z=${tz}&p=${tp}&r=${tr}`)
                .then(r => {
                    if (r.ok) {
                        document.getElementById('connDot').className = "w-2 h-2 rounded-full bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.8)]";
                        document.getElementById('connText').innerText = "Hardware Synced";
                    }
                })
                .catch(e => {
                    document.getElementById('connDot').className = "w-2 h-2 rounded-full bg-red-500 shadow-[0_0_8px_rgba(239,68,68,0.8)]";
                    document.getElementById('connText').innerText = "Disconnected";
                });
            }, 50); 
        }

        function wave() {
            setPose(0, -70, 28, -35, 0); // Sit
            setTimeout(() => { fetch('/anim?mode=1'); }, 2000); // Wait 2 seconds for robot to sit securely before lifting leg
        }

        function setPose(x, y, z, p, r) {
            fetch('/anim?mode=0'); // Clear any animation
            document.getElementById('sliderX').value = x;
            document.getElementById('sliderY').value = y;
            document.getElementById('sliderZ').value = z;
            document.getElementById('sliderPitch').value = p;
            document.getElementById('sliderRoll').value = r;
            document.getElementById('valX').innerText = x + ' mm';
            document.getElementById('valY').innerText = y + ' mm';
            document.getElementById('valZ').innerText = z + ' mm';
            document.getElementById('valPitch').innerText = p + '\xB0';
            document.getElementById('valRoll').innerText = r + '\xB0';
            updateSimulation();
            sendToESP32(x, y, z, p, r);
        }

        function updateSimulation() {
            const tx = parseFloat(document.getElementById('sliderX').value);
            const ty = parseFloat(document.getElementById('sliderY').value);
            const tz = parseFloat(document.getElementById('sliderZ').value);
            const tp = parseFloat(document.getElementById('sliderPitch').value);
            const tr = parseFloat(document.getElementById('sliderRoll').value);

            document.getElementById('valX').innerText = tx + ' mm';
            document.getElementById('valY').innerText = ty + ' mm';
            document.getElementById('valZ').innerText = tz + ' mm';
            document.getElementById('valPitch').innerText = tp + '\xB0';
            document.getElementById('valRoll').innerText = tr + '\xB0';

            targetMarker.position.set(tx, ty, tz);
            const ik = solveDogIK(tx, ty, tz);

            coxaGroup.rotation.x = -ik.rad.c; 
            femurGroup.rotation.z = ik.rad.f; 
            tibiaGroup.rotation.z = -ik.rad.t; 

            sendToESP32(tx, ty, tz, tp, tr);
        }

        ['X', 'Y', 'Z', 'Pitch', 'Roll'].forEach(axis => {
            document.getElementById('slider' + axis).addEventListener('input', updateSimulation);
        });

        function updatePID() {
            const p = parseFloat(document.getElementById('sliderKp').value);
            const i = parseFloat(document.getElementById('sliderKi').value);
            const d = parseFloat(document.getElementById('sliderKd').value);
            document.getElementById('valKp').innerText = p.toFixed(2);
            document.getElementById('valKi').innerText = i.toFixed(2);
            document.getElementById('valKd').innerText = d.toFixed(2);
            fetch(`/pid?p=${p}&i=${i}&d=${d}`);
        }

        document.querySelectorAll('.pid-slider').forEach(s => s.addEventListener('input', updatePID));

        document.getElementById('sliderDB').addEventListener('input', (e) => {
            const v = parseFloat(e.target.value);
            document.getElementById('valDB').innerHTML = v.toFixed(1) + '&deg;';
            fetch(`/deadband?v=${v}`);
        });

        window.addEventListener('resize', () => {
            camera.aspect = window.innerWidth / window.innerHeight;
            camera.updateProjectionMatrix();
            renderer.setSize(window.innerWidth, window.innerHeight);
        });

        window.onload = function () {
            initJoystick('joyLeft', true);
            initJoystick('joyRight', false);
            syncToggles(); // set initial state
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
