<template>
  <div class="universe">
    <div id="canvas-container"></div>
    <el-scrollbar class="control-panel">
      <div class="algorithm-selector">
        <label>
          <input type="radio" v-model="algorithm" value="newton" :disabled="isSimulating">
          经典牛顿算法 (O(n²))
        </label>
        <label>
          <input type="radio" v-model="algorithm" value="barnes-hut" :disabled="isSimulating">
          Barnes-Hut算法 (O(n log n))
        </label>
      </div>

      <!-- 引力场控制面板 -->
      <div class="field-control">
        <h3>引力场可视化</h3>
        <div class="field-toggle">
          <button @click="toggleGravitationalField" :class="{ active: showGravitationalField }">
            {{ showGravitationalField ? '隐藏引力场' : '显示引力场' }}
          </button>
        </div>
        
        <div class="field-settings" v-if="showGravitationalField">
          <div class="field-resolution">
            <label>分辨率</label>
            <input 
              type="range" 
              v-model.number="fieldResolution" 
              :min="3" 
              :max="10" 
              :disabled="isSimulating"
              @change="updateGravitationalField"
            >
            <span>{{ fieldResolution }}</span>
          </div>

          <div class="field-size">
            <label>显示范围 (AU)</label>
            <input 
              type="number" 
              v-model.number="fieldSizeAU" 
              :min="1" 
              :max="100" 
              :disabled="isSimulating"
              @change="updateFieldSize"
            >
          </div>
        </div>
      </div>

      <!-- 轨迹控制面板 -->
      <div class="field-control">
        <h3>天体轨迹</h3>
        <div class="field-toggle">
          <button @click="toggleTrajectories" :class="{ active: showTrajectories }">
            {{ showTrajectories ? '隐藏轨迹' : '显示轨迹' }}
          </button>
        </div>
      </div>

      <!-- 时间控制面板 -->
      <div class="time-control">
        <h3>时间控制</h3>
        <div class="time-direction">
          <label>
            <input type="radio" v-model="timeDirection" value="forward" :disabled="isSimulating">
            正向推进
          </label>
          <label>
            <input type="radio" v-model="timeDirection" value="backward" :disabled="isSimulating">
            逆向反演
          </label>
        </div>
        
        <div class="time-step">
          <label>时间步长 (秒)</label>
          <input 
            type="number" 
            v-model.number="timeStep" 
            :min="1" 
            :max="86400" 
            :disabled="isSimulating"
          >
        </div>

        <div class="time-jump">
          <label>时间跳转 (天)</label>
          <div class="time-jump-controls">
            <input 
              type="number" 
              v-model.number="jumpDays" 
              :min="0" 
              :disabled="isSimulating"
            >
            <button @click="jumpTime" :disabled="isSimulating">跳转</button>
          </div>
        </div>
      </div>

      <div style="display: flex">
        <button @click="toggleSimulation" :class="{ active: isSimulating }" style="width: 100%;">
          {{ isSimulating ? '暂停模拟' : '开始模拟' }}
        </button>
        <button @click="toggleRecording" :class="{ active: isRecording }" style="width: 100%;">
          {{ isRecording ? '暂停录制' : '开始录制' }}
        </button>
      </div>

      <!-- 添加导入导出按钮 -->
      <div class="io-controls">
        <button @click="exportConfig">导出配置</button>
        <button @click="importConfig">导入配置</button>
        <input
          type="file"
          ref="fileInput"
          style="display: none"
          accept=".json"
          @change="handleFileUpload"
        >
      </div>

      <div class="debug-info">
        <h3>调试信息</h3>
        <div v-for="(body, name) in debugInfo" :key="name">
          <h4>{{ name }}</h4>
          <p>位置: {{ formatVector(body.position) }}</p>
          <p>速度: {{ formatVector(body.velocity) }}</p>
          <p>加速度: {{ formatVector(body.acceleration) }}</p>
        </div>
      </div>

      <div class="debug-info">
        <h3>事件日志</h3>
        <!-- 可以增加一个手动刷新按钮 -->
        <button @click="fetchEvents" :disabled="isSimulating">刷新事件日志</button>

        <ul>
          <li v-for="(ev, index) in events" :key="index">
            <!-- 简单示例: 打印事件类型与message -->
            [{{ ev.type }}] {{ ev.message }}
          </li>
        </ul>
      </div>
    </el-scrollbar>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, onBeforeUnmount, watch } from 'vue';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';
import axios from 'axios';

// 响应式状态变量
const width = ref(window.innerWidth);
const height = ref(window.innerHeight);
const isSimulating = ref(false);
const isRecording = ref(false);
const simulationInterval = ref(null);
const algorithm = ref('newton'); // 默认使用牛顿算法
const debugInfo = reactive({});
let events = reactive([]);
const timeDirection = ref('forward');
const timeStep = ref(3600);
const jumpDays = ref(0);
const showGravitationalField = ref(false);
const showTrajectories = ref(false); // 新增：控制是否显示轨迹
const fieldArrows = ref([]); // 存储引力场箭头对象
const fieldResolution = ref(8);
const fieldSizeAU = ref(3);
const fieldSize = ref(3 * 1.496e11); // 将天文单位转换为米
const fieldArrowScale = ref(5);
const fieldArrowColor = ref('#00ff00'); // 使用十六进制颜色表示

// 存储天体对象的映射
const celestialBodies = new Map();

// 存储轨迹线的映射
const trajectories = new Map();

// Three.js 相关变量
let scene = null;
let camera = null;
let renderer = null;
let controls = null;
let isInitialized = false;

// 格式化向量以用于调试显示
const formatVector = (vec) => {
  if (!vec || !Array.isArray(vec)) return 'N/A';
  return vec.map(v => v.toExponential(2)).join(', ');
};

// 根据天体名称创建材质
const createCelestialBodyMaterial = (bodyName) => {
  switch(bodyName) {
    case 'Sun':
      return new THREE.MeshBasicMaterial({
        color: 0xffff00,
        emissive: 0xffaa00,
        emissiveIntensity: 1.5,
        map: new THREE.TextureLoader().load('/Sun.png')
      });
    case 'Mercury':
      return new THREE.MeshStandardMaterial({
        // color: 0x8B8B83,
        // metalness: 0.6,
        // roughness: 0.4,
        // emissive: 0x8B8B83,
        // emissiveIntensity: 0.2,
        map: new THREE.TextureLoader().load('/Mercury.png')
      });
    case 'Venus':
      return new THREE.MeshStandardMaterial({
        // color: 0xE7C697,
        // metalness: 0.3,
        // roughness: 0.6,
        // emissive: 0xE7C697,
        // emissiveIntensity: 0.2
        map: new THREE.TextureLoader().load('/Venus.jpg')
      });
    case 'Earth':
      return new THREE.MeshStandardMaterial({
        // color: 0x4169E1,
        // metalness: 0.3,
        // roughness: 0.4,
        // emissive: 0x4169E1,
        // emissiveIntensity: 0.2,
        map:new THREE.TextureLoader().load('/Earth.png')
      });
    case 'Mars':
      return new THREE.MeshStandardMaterial({
        // color: 0xA0522D,
        // metalness: 0.3,
        // roughness: 0.5,
        // emissive: 0xA0522D,
        // emissiveIntensity: 0.2
        map: new THREE.TextureLoader().load('/Mars.jpg')
      });
    case 'Jupiter':
      return new THREE.MeshStandardMaterial({
        // color: 0xDAA520,
        // metalness: 0.3,
        // roughness: 0.4,
        // emissive: 0xDAA520,
        // emissiveIntensity: 0.2
        map: new THREE.TextureLoader().load('/Jupiter.jpg')
      });
    case 'Saturn':
      return new THREE.MeshStandardMaterial({
        // color: 0xCD853F,
        // metalness: 0.4,
        // roughness: 0.4,
        // emissive: 0xCD853F,
        // emissiveIntensity: 0.2
        map: new THREE.TextureLoader().load('/Saturn.jpg')
      });
    case 'Uranus':
      return new THREE.MeshStandardMaterial({
        // color: 0x4682B4,
        // metalness: 0.5,
        // roughness: 0.3,
        // emissive: 0x4682B4,
        // emissiveIntensity: 0.2
        map: new THREE.TextureLoader().load('/天王星.jpg')
      });
    case 'Neptune':
      return new THREE.MeshStandardMaterial({
        // color: 0x4169E1,
        // metalness: 0.5,
        // roughness: 0.3,
        // emissive: 0x4169E1,
        // emissiveIntensity: 0.2
        map: new THREE.TextureLoader().load('/海王星.jpg')
      });
    default:
      return new THREE.MeshStandardMaterial({
        color: 0x808080
      });
  }
};

// 创建轨迹线
const createTrajectory = (bodyName) => {
  const geometry = new THREE.BufferGeometry();
  const maxPoints = 1000; // 最大轨迹点数
  const positions = new Float32Array(maxPoints * 3); // 每个点有x, y, z
  geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  geometry.setDrawRange(0, 0); // 初始不绘制任何点

  const material = new THREE.LineBasicMaterial({
    color: 0xffffff, // 轨迹颜色，可以根据需要修改
    transparent: true,
    opacity: 0.6
  });

  const line = new THREE.Line(geometry, material);
  scene.add(line);
  trajectories.set(bodyName, { line, positions, currentIndex: 0, maxPoints });
};

// 更新轨迹线
const updateTrajectory = (bodyName, position) => {
  const traj = trajectories.get(bodyName);
  if (!traj) return;

  const { positions, currentIndex, maxPoints } = traj;

  positions[currentIndex * 3] = position.x;
  positions[currentIndex * 3 + 1] = position.y;
  positions[currentIndex * 3 + 2] = position.z;

  traj.currentIndex = (currentIndex + 1) % maxPoints;
  const drawCount = Math.min(traj.currentIndex, maxPoints);
  traj.line.geometry.setDrawRange(0, drawCount);
  traj.line.geometry.attributes.position.needsUpdate = true;
};

// 更新场景中的天体
const updateCelestialBodies = (bodies) => {
  bodies.forEach(bodyData => {
    let body = celestialBodies.get(bodyData.name);
    
    if (!body) {
      // 根据天体名称调整缩放比例
      const radiusScale = bodyData.name === 'Sun' ? 5e-8 : 1e-6;
      const geometry = new THREE.SphereGeometry(
        bodyData.radius * radiusScale,
        32,
        32
      );
      const material = createCelestialBodyMaterial(bodyData.name);
      body = new THREE.Mesh(geometry, material);
      scene.add(body);
      celestialBodies.set(bodyData.name, body);

      // 如果显示轨迹，则创建轨迹线
      if (showTrajectories.value) {
        createTrajectory(bodyData.name);
      }
    }

    // 调整位置缩放比例
    const scale = 1e-9;
    const pos = new THREE.Vector3(
      bodyData.position[0] * scale,
      bodyData.position[1] * scale,
      bodyData.position[2] * scale
    );
    body.position.copy(pos);

    // 更新轨迹
    if (showTrajectories.value) {
      updateTrajectory(bodyData.name, pos);
    }
  });

  // 移除不再存在的天体及其轨迹
  const newBodyNames = new Set(bodies.map(b => b.name));
  celestialBodies.forEach((body, name) => {
    if (!newBodyNames.has(name)) {
      scene.remove(body);
      celestialBodies.delete(name);

      // 移除对应的轨迹线
      const traj = trajectories.get(name);
      if (traj) {
        scene.remove(traj.line);
        traj.line.geometry.dispose();
        traj.line.material.dispose();
        trajectories.delete(name);
      }
    }
  });
};

// 更新调试信息
const updateDebugInfoFn = (bodies) => {
  Object.keys(debugInfo).forEach(key => {
    delete debugInfo[key];
  });
  bodies.forEach(body => {
    debugInfo[body.name] = {
      position: body.position,
      velocity: body.velocity,
      acceleration: body.acceleration
    };
  });
};

// 获取事件日志
const fetchEvents = async () => {
  try {
    // 根据选择的算法来决定是否加上 "?algorithm=barnes-hut"
    const response = await axios.get(`http://localhost:8081/api/events${algorithm.value === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`);
    events = response.data.events;

    // const mockEvents = [
    //   {
    //     type: 'collision',
    //     time: 123456,
    //     bodies: ['Earth', 'Mars'],
    //     distance: 1.23e6,
    //     message: 'Collision occurred between Earth and Mars!'
    //   },
    //   {
    //     type: 'eclipse',
    //     time: 789012,
    //     bodies: ['Sun', 'Moon', 'Earth'],
    //     distance: 3.45e5,
    //     message: 'Eclipse event: Moon shadow on Earth.'
    //   }
    // ];
    // // 将这些假数据直接赋给 events
    // events = mockEvents;
    console.log('事件日志:', events);

  } catch (error) {
    console.error('获取事件日志时出错:', error);
  }
};

const removeBody = async (bodyName) => {
  try {
    const response = await axios.post(
        `http://localhost:8081/api/remove${algorithm.value === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`,
        {
          name: bodyName
        }
    );
    updateCelestialBodies(response.data);
    updateDebugInfoFn(response.data);
  } catch (error) {
    console.error('跳跃时间时出错:', error);
  }
};

//前端检测碰撞
const detectCollisionsFrontEnd = () => {
  console.log("执行检测碰撞方法")

  // 将 celestialBodies (Map) 转成数组，方便双重循环
  const bodiesArray = Array.from(celestialBodies.entries());
  // bodiesArray[i] = [bodyName, mesh]

  const toRemove = new Set(); // 用于存储需要移除的天体名称

  for (let i = 0; i < bodiesArray.length; i++) {
    for (let j = i + 1; j < bodiesArray.length; j++) {
      const [nameA, meshA] = bodiesArray[i];
      const [nameB, meshB] = bodiesArray[j];

      // 通过 mesh.geometry.parameters.radius 拿到可视半径
      const radiusA = meshA.geometry?.parameters?.radius || 0;
      const radiusB = meshB.geometry?.parameters?.radius || 0;

      const posA = meshA.position;
      const posB = meshB.position;

      const dx = posA.x - posB.x;
      const dy = posA.y - posB.y;
      const dz = posA.z - posB.z;
      const distSq = dx * dx + dy * dy + dz * dz;

      const collisionDist = radiusA + radiusB;

      if (distSq < collisionDist * collisionDist) {
        console.log("发生碰撞----------------------------------------------------")
        events.push({
          type: "collision",
          time: new Date().toLocaleTimeString(),
          bodies: [nameA, nameB],
          message: `${nameA} 和 ${nameB} 发生碰撞！`
        });

        toRemove.add(nameA);
        toRemove.add(nameB);
      }
    }
  }

  if (toRemove.size > 0) {
    toRemove.forEach(name => {
      removeBody(name)
      console.log("移除天体" + name);
    });
  }
};

// 获取初始系统状态
const fetchInitialState = async () => {
  try {
    const response = await axios.get(`http://localhost:8081/api/system-state${algorithm.value === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`);
    updateCelestialBodies(response.data);
    updateDebugInfoFn(response.data);
  } catch (error) {
    console.error('获取初始状态时出错:', error);
  }
};

// 模拟一步
const simulateStep = async () => {
  try {
    console.log('模拟一步...');
    const response = await axios.post(
      `http://localhost:8081/api/simulate${algorithm.value === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`,
      {
        timeDirection: timeDirection.value,
        timeStep: timeStep.value
      }
    );
    console.log('收到模拟数据:', response.data);
    updateCelestialBodies(response.data);
    updateDebugInfoFn(response.data);
    // await fetchEvents();
    detectCollisionsFrontEnd();

    // 如果引力场可视化开启，则更新引力场
    if (showGravitationalField.value) {
      await updateGravitationalField();
    }
  } catch (error) {
    console.error('模拟步骤时出错:', error);
  }

};

// 切换模拟状态
const toggleSimulation = () => {
  isSimulating.value = !isSimulating.value;
  if (isSimulating.value) {
    // 根据引力场可视化状态设置更新间隔
    const updateInterval = showGravitationalField.value ? 25 : 50;
    simulationInterval.value = setInterval(simulateStep, updateInterval);
  } else {
    clearInterval(simulationInterval.value);
  }
};

const toggleRecording = () => {
  if (isRecording.value) {
    stopRecording();
  } else {
    startRecording();
  }
  isRecording.value = !isRecording.value;
};

// 初始化 Three.js 场景
const init = () => {
  if (isInitialized) return;
  
  // 创建场景
  scene = new THREE.Scene();
  
  // 创建渐变背景
  const bgColor1 = new THREE.Color(0x000020); 
  const bgColor2 = new THREE.Color(0x110024); 
  const canvas = document.createElement('canvas');
  const context = canvas.getContext('2d');
  canvas.width = 2;
  canvas.height = 2;
  
  const gradient = context.createLinearGradient(0, 0, 0, 2);
  gradient.addColorStop(0, `#${bgColor1.getHexString()}`);
  gradient.addColorStop(1, `#${bgColor2.getHexString()}`);
  
  context.fillStyle = gradient;
  context.fillRect(0, 0, 2, 2);
  
  const bgTexture = new THREE.CanvasTexture(canvas);
  scene.background = bgTexture;

  // const loader = new THREE.TextureLoader();
  // loader.load('/Background2.jpg', (texture) => {
  //   scene.background = texture;
  //   console.log('背景贴图加载成功--------------------------------');
  // });

  // 设置相机参数
  camera = new THREE.PerspectiveCamera(
    75,
    width.value / height.value,
    0.001,
    10000
  );
  camera.position.set(0, 200, 200);
  camera.lookAt(0, 0, 0);

  // 初始化渲染器
  renderer = new THREE.WebGLRenderer({
    antialias: true,
    powerPreference: "high-performance",
    alpha: true
  });
  renderer.setSize(width.value, height.value);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.5;  // 增加曝光
  
  // 将渲染器添加到 DOM
  const container = document.getElementById('canvas-container');
  if (container) {
    container.appendChild(renderer.domElement);
  }

  // 初始化控制器
  controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.dampingFactor = 0.05;
  controls.screenSpacePanning = false;
  controls.minDistance = 1;
  controls.maxDistance = 1000;
  controls.autoRotate = false;
  controls.enableRotate = true;
  controls.maxPolarAngle = Math.PI / 2;  // 限制相机不能移动到星系下方
  controls.minPolarAngle = 0;  // 允许相机移动到正上方

  // 增强光照
  const ambientLight = new THREE.AmbientLight(0x404040, 1.0);  // 增加环境光强度
  scene.add(ambientLight);

  // 添加太阳点光源
  const sunLight = new THREE.PointLight(0xffffff, 2, 50);
  sunLight.position.set(0, 0, 0);
  scene.add(sunLight);

  // 添加半球光
  const hemisphereLight = new THREE.HemisphereLight(0xffffbb, 0x080820, 1);
  scene.add(hemisphereLight);

  // 监听窗口大小变化
  window.addEventListener('resize', onWindowResize, false);
  
  isInitialized = true;
  renderScene();
};

// 渲染场景
const renderScene = () => {
  if (!isInitialized) return;
  
  if (controls) {
    controls.update();
  }

  if (renderer && scene && camera) {
    renderer.render(scene, camera);
  }

  requestAnimationFrame(renderScene);
};

// 模拟和录制的控制
let mediaRecorder;
let recordedChunks = [];
const startRecording = () => {
  const canvas = renderer.domElement;  // 获取 Three.js 的渲染画布
  const stream = canvas.captureStream(60);  // 捕获流，参数是帧率

  mediaRecorder = new MediaRecorder(stream, {
    mimeType: 'video/webm; codecs=vp9'
  });
  recordedChunks = [];

  mediaRecorder.ondataavailable = function(event) {
    if (event.data.size > 0) {
      recordedChunks.push(event.data);
    }
  };

  mediaRecorder.onstop = saveVideo;
  mediaRecorder.start();
  toggleSimulation();
};

const stopRecording = () => {
  mediaRecorder.stop();
  toggleSimulation();
};

const saveVideo = () => {
  const blob = new Blob(recordedChunks, {
    type: "video/webm"
  });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.style.display = "none";
  a.href = url;
  a.download = "simulation.webm";
  document.body.appendChild(a);
  a.click();
  setTimeout(() => {
    window.URL.revokeObjectURL(url);
    document.body.removeChild(a);
  }, 100);
};

// 处理窗口大小变化
const onWindowResize = () => {
  width.value = window.innerWidth;
  height.value = window.innerHeight;
  
  if (camera) {
    camera.aspect = width.value / height.value;
    camera.updateProjectionMatrix();
  }
  
  if (renderer) {
    renderer.setSize(width.value, height.value);
  }
};

// 导出配置
const exportConfig = async () => {
  try {
    const response = await axios.get(
      `http://localhost:8081/api/export-config${algorithm.value === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`
    );
    
    // 创建下载文件
    const blob = new Blob([JSON.stringify(response.data, null, 2)], { type: 'application/json' });
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'solar-system-config.json';
    document.body.appendChild(a);
    a.click();
    window.URL.revokeObjectURL(url);
    document.body.removeChild(a);
  } catch (error) {
    console.error('导出配置时出错:', error);
  }
};

// 触发文件输入以导入配置
const importConfig = () => {
  fileInput.value.click();
};

// 隐藏的文件输入引用
const fileInput = ref(null);

// 处理文件上传
const handleFileUpload = async (event) => {
  const file = event.target.files[0];
  if (!file) return;

  try {
    const reader = new FileReader();
    reader.onload = async (e) => {
      const config = JSON.parse(e.target.result);
      
      // 发送配置到服务器
      await axios.post(
        `http://localhost:8081/api/import-config${algorithm.value === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`,
        config
      );
      
      // 重新获取系统状态并更新场景
      await fetchInitialState();

      // 清除现有的天体和轨迹
      celestialBodies.forEach((body) => {
        scene.remove(body);
      });
      celestialBodies.clear();

      trajectories.forEach((traj) => {
        scene.remove(traj.line);
        traj.line.geometry.dispose();
        traj.line.material.dispose();
      });
      trajectories.clear();

      // 重新创建天体
      if (config.bodies) {
        updateCelestialBodies(config.bodies);
      }

      // 重置相机位置以查看新的配置
      if (camera) {
        camera.position.set(0, 200, 200);
        camera.lookAt(0, 0, 0);
      }
    };
    reader.readAsText(file);
  } catch (error) {
    console.error('导入配置时出错:', error);
  } finally {
    // 清除文件输入，允许重复上传同一个文件
    event.target.value = '';
  }
};

// 跳跃时间
const jumpTime = async () => {
  if (jumpDays.value <= 0) return;
  
  try {
    const response = await axios.post(
      `http://localhost:8081/api/jump-time${algorithm.value === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`,
      {
        days: jumpDays.value
      }
    );
    updateCelestialBodies(response.data);
    updateDebugInfoFn(response.data);
  } catch (error) {
    console.error('跳跃时间时出错:', error);
  }
};

// 切换引力场可视化
const toggleGravitationalField = async () => {
  // 先清除现有的引力场
  clearGravitationalField();
  
  showGravitationalField.value = !showGravitationalField.value;
  
  if (showGravitationalField.value) {
    await updateGravitationalField();
    // 如果正在模拟，调整更新频率
    if (isSimulating.value) {
      clearInterval(simulationInterval.value);
      simulationInterval.value = setInterval(simulateStep, 500);
    }
  } else {
    // 如果正在模拟，恢复正常更新频率
    if (isSimulating.value) {
      clearInterval(simulationInterval.value);
      simulationInterval.value = setInterval(simulateStep, 1000);
    }
  }
};

// 切换轨迹可视化
const toggleTrajectories = () => {
  showTrajectories.value = !showTrajectories.value;

  if (showTrajectories.value) {
    // 为现有天体创建轨迹线
    celestialBodies.forEach((body, name) => {
      if (!trajectories.has(name)) {
        createTrajectory(name);
      }
    });
  } else {
    // 移除所有轨迹线
    // eslint-disable-next-line
    trajectories.forEach((traj, name) => {
      scene.remove(traj.line);
      traj.line.geometry.dispose();
      traj.line.material.dispose();
    });
    trajectories.clear();
  }
};

// 清除引力场
const clearGravitationalField = () => {
  if (fieldArrows.value.length > 0) {
    fieldArrows.value.forEach(arrow => {
      if (arrow && scene) {
        if (arrow.cone) {
          scene.remove(arrow.cone);
          arrow.cone.geometry.dispose();
          arrow.cone.material.dispose();
        }
        if (arrow.line) {
          scene.remove(arrow.line);
          arrow.line.geometry.dispose();
          arrow.line.material.dispose();
        }
        scene.remove(arrow);
      }
    });
    // 清空数组
    fieldArrows.value = [];
  }
  // 强制渲染器更新
  if (renderer) {
    renderer.renderLists.dispose();
  }
};

// 更新引力场可视化
const updateGravitationalField = async () => {
  try {
    // 确保先清除现有的引力场
    clearGravitationalField();
    
    // 等待一帧以确保清除完成
    await new Promise(resolve => requestAnimationFrame(resolve));

    // 如果引力场显示被关闭，直接返回
    if (!showGravitationalField.value) {
      return;
    }

    // 获取引力场数据
    const response = await axios.get(
      `http://localhost:8081/api/gravitational-field${algorithm.value === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`,
      {
        params: {
          centerX: 0,
          centerY: 0,
          centerZ: 0,
          size: fieldSize.value,
          resolution: fieldResolution.value
        }
      }
    );

    const fieldData = response.data;
    const scale = 1e-9;  // 缩放因子，与天体位置缩放一致

    // 创建引力场箭头
    fieldData.forEach(point => {
      const position = new THREE.Vector3(
        point.position[0] * scale,
        point.position[1] * scale,
        point.position[2] * scale
      );

      const direction = new THREE.Vector3(
        -point.field[0],  // 反转方向，使箭头指向引力源
        -point.field[1],
        -point.field[2]
      ).normalize();

      // 根据引力场强度调整箭头长度
      const length = Math.log10(point.magnitude) * fieldArrowScale.value;

      // 创建箭头
      const arrowHelper = new THREE.ArrowHelper(
        direction,
        position,
        length,
        fieldArrowColor.value,
        length * 0.3,  // 增大箭头头部长度
        length * 0.2   // 增大箭头头部宽度
      );

      // 设置箭头材质为发光材质
      const arrowMaterial = new THREE.MeshBasicMaterial({
        color: fieldArrowColor.value,
        transparent: true,
        opacity: 0.8,
        emissive: fieldArrowColor.value,
        emissiveIntensity: 1.0
      });
      
      if (arrowHelper.cone) {
        arrowHelper.cone.material = arrowMaterial;
      }
      if (arrowHelper.line) {
        arrowHelper.line.material = arrowMaterial;
      }

      scene.add(arrowHelper);
      fieldArrows.value.push(arrowHelper);
    });

    // 添加引力场强度指示器
    const maxMagnitude = Math.max(...fieldData.map(point => point.magnitude));
    console.log('最大引力场强度:', maxMagnitude);
  } catch (error) {
    console.error('更新引力场时出错:', error);
  }
};


// 监听算法变化以更新引力场
watch(algorithm, () => {
  if (showGravitationalField.value) {
    updateGravitationalField();
  }
});

// 生命周期钩子
onMounted(() => {
  init();
  fetchInitialState();
});

onBeforeUnmount(() => {
  if (simulationInterval.value) {
    clearInterval(simulationInterval.value);
  }

  isInitialized = false;
  window.removeEventListener('resize', onWindowResize);

  // 清理 Three.js 资源
  if (renderer) {
    renderer.dispose();
    if (renderer.domElement && renderer.domElement.parentNode) {
      renderer.domElement.parentNode.removeChild(renderer.domElement);
    }
  }

  if (controls) {
    controls.dispose();
  }

  celestialBodies.forEach((body) => {
    scene.remove(body);
  });
  celestialBodies.clear();

  trajectories.forEach((traj) => {
    scene.remove(traj.line);
    traj.line.geometry.dispose();
    traj.line.material.dispose();
  });
  trajectories.clear();

  scene = null;
  camera = null;
  renderer = null;
  controls = null;
});
</script>

<style scoped>
.universe {
  width: 100%;
  height: 100vh;
  margin: 0;
  padding: 0;
  overflow: hidden;
  background-color: #000;
  position: relative;
}

#canvas-container {
  width: 100%;
  height: 100%;
}

.control-panel {
  box-sizing: border-box;
  position: absolute;
  top: 20px;
  right: 20px;
  bottom: 20px;
  padding: 15px;
  background-color: rgba(0, 0, 0, 0.7);
  border-radius: 8px;
  display: flex;
  flex-direction: column;
  gap: 15px;
}

.algorithm-selector {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.algorithm-selector label {
  color: white;
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
}

.algorithm-selector input[type="radio"] {
  cursor: pointer;
}

.algorithm-selector input[type="radio"]:disabled {
  cursor: not-allowed;
}

button {
  padding: 10px 20px;
  background-color: rgba(255, 255, 255, 0.2);
  border: 1px solid rgba(255, 255, 255, 0.3);
  color: white;
  border-radius: 4px;
  cursor: pointer;
  transition: background-color 0.3s;
}

button:hover {
  background-color: rgba(255, 255, 255, 0.3);
}

button.active {
  background-color: rgba(0, 255, 0, 0.3);
}

.debug-info {
  margin-top: 15px;
  margin-bottom: 15px;
  padding: 10px;
  background-color: rgba(0, 0, 0, 0.8);
  border-radius: 4px;
  color: white;
  font-family: monospace;
  max-height: 300px;
  overflow-y: auto;
}

.debug-info h3 {
  margin: 0 0 10px 0;
  color: #4CAF50;
}

.debug-info h4 {
  margin: 10px 0 5px 0;
  color: #2196F3;
}

.debug-info p {
  margin: 2px 0;
  font-size: 12px;
}

.io-controls {
  display: flex;
  gap: 10px;
  margin: 10px 0;
}

.io-controls button {
  flex: 1;
  padding: 8px;
  background-color: rgba(255, 255, 255, 0.2);
  border: 1px solid rgba(255, 255, 255, 0.3);
  color: white;
  border-radius: 4px;
  cursor: pointer;
  transition: background-color 0.3s;
}

.io-controls button:hover {
  background-color: rgba(255, 255, 255, 0.3);
}

.trajectory-control{
  margin-top: 20px;
}

.trajectory-toggle {
  margin-bottom: 10px;
}

.time-control {
  background-color: rgba(0, 0, 0, 0.8);
  padding: 15px;
  border-radius: 8px;
  margin: 10px 0;
}

.time-control h3 {
  margin: 0 0 10px 0;
  color: #4CAF50;
}

.time-direction {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-bottom: 15px;
}

.time-direction label {
  color: white;
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
}

.time-step {
  margin-bottom: 15px;
}

.time-step label {
  display: block;
  color: white;
  margin-bottom: 5px;
}

.time-step input,
.time-jump input {
  width: 100%;
  padding: 5px;
  background-color: rgba(255, 255, 255, 0.1);
  border: 1px solid rgba(255, 255, 255, 0.3);
  color: white;
  border-radius: 4px;
}

.time-jump label {
  display: block;
  color: white;
  margin-bottom: 5px;
}

.time-jump-controls {
  display: flex;
  gap: 10px;
}

.time-jump-controls input {
  flex: 1;
}

.time-jump-controls button {
  padding: 5px 10px;
}

.field-control {
  background-color: rgba(0, 0, 0, 0.8);
  padding: 15px;
  border-radius: 8px;
  margin: 10px 0;
}

.field-control h3 {
  margin: 0 0 10px 0;
  color: #4CAF50;
}

.field-toggle button {
  width: 100%;
  padding: 8px;
  margin-bottom: 10px;
}

.field-toggle button.active {
  background-color: rgba(0, 255, 0, 0.3);
}

.field-settings {
  margin-top: 10px;
}

.field-resolution,
.field-size {
  margin-bottom: 10px;
}

.field-resolution label,
.field-size label {
  display: block;
  color: white;
  margin-bottom: 5px;
}

.field-resolution input[type="range"] {
  width: calc(100% - 30px);
  margin-right: 10px;
}

.field-resolution span {
  color: white;
}

.field-size input {
  width: 100%;
  padding: 5px;
  background-color: rgba(255, 255, 255, 0.1);
  border: 1px solid rgba(255, 255, 255, 0.3);
  color: white;
  border-radius: 4px;
}
</style>
