<template>
  <div class="universe">
    <div id="canvas-container"></div>
    <div class="control-panel">
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
      <button @click="toggleSimulation" :class="{ active: isSimulating }">
        {{ isSimulating ? '暂停模拟' : '开始模拟' }}
      </button>

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
    </div>
  </div>
</template>

<script>
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';
import axios from 'axios';

let scene = null;
let camera = null;
let renderer = null;
let controls = null;
let isInitialized = false;
let celestialBodies = new Map(); // 存储天体对象的映射

export default {
  name: 'App',
  data() {
    return {
      width: window.innerWidth,
      height: window.innerHeight,
      isSimulating: false,
      simulationInterval: null,
      algorithm: 'newton', // 默认使用牛顿算法
      debugInfo: {} // 存储调试信息
    };
  },
  mounted() {
    this.init();
    this.fetchInitialState();
  },
  methods: {
    formatVector(vec) {
      if (!vec || !Array.isArray(vec)) return 'N/A';
      return vec.map(v => v.toExponential(2)).join(', ');
    },

    async fetchInitialState() {
      try {
        const response = await axios.get(`http://localhost:8081/api/system-state${this.algorithm === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`);
        this.updateCelestialBodies(response.data);
        this.updateDebugInfo(response.data);
      } catch (error) {
        console.error('Error fetching initial state:', error);
      }
    },

    async simulateStep() {
      try {
        console.log('Simulating step...');
        const response = await axios.post(`http://localhost:8081/api/simulate${this.algorithm === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`);
        console.log('Received simulation data:', response.data);
        this.updateCelestialBodies(response.data);
        this.updateDebugInfo(response.data);
      } catch (error) {
        console.error('Error in simulation step:', error);
      }
    },

    updateDebugInfo(bodies) {
      const newDebugInfo = {};
      bodies.forEach(body => {
        newDebugInfo[body.name] = {
          position: body.position,
          velocity: body.velocity,
          acceleration: body.acceleration
        };
      });
      this.debugInfo = newDebugInfo;
    },

    toggleSimulation() {
      this.isSimulating = !this.isSimulating;
      if (this.isSimulating) {
        this.simulationInterval = setInterval(this.simulateStep, 1000); // 每秒更新一次
      } else {
        clearInterval(this.simulationInterval);
      }
    },

    updateCelestialBodies(bodies) {
      bodies.forEach(bodyData => {
        let body = celestialBodies.get(bodyData.name);
        
        if (!body) {
          // 调整天体大小的缩放比例
          const radiusScale = bodyData.name === 'Sun' ? 1e-8 : 1e-6;
          const geometry = new THREE.SphereGeometry(
            bodyData.radius * radiusScale,
            32,
            32
          );
          const material = this.createCelestialBodyMaterial(bodyData.name);
          body = new THREE.Mesh(geometry, material);
          scene.add(body);
          celestialBodies.set(bodyData.name, body);
        }

        // 调整位置的缩放比例
        const scale = 1e-9;
        body.position.set(
          bodyData.position[0] * scale,
          bodyData.position[1] * scale,
          bodyData.position[2] * scale
        );
      });

      // 移除不再存在的天体
      const newBodyNames = new Set(bodies.map(b => b.name));
      celestialBodies.forEach((body, name) => {
        if (!newBodyNames.has(name)) {
          scene.remove(body);
          celestialBodies.delete(name);
        }
      });
    },

    createCelestialBodyMaterial(bodyName) {
      switch(bodyName) {
        case 'Sun':
          return new THREE.MeshBasicMaterial({
            color: 0xffff00,
            emissive: 0xffaa00,
            emissiveIntensity: 1.5
          });
        case 'Mercury':
          return new THREE.MeshStandardMaterial({
            color: 0x8B8B83,
            metalness: 0.6,
            roughness: 0.4,
            emissive: 0x8B8B83,
            emissiveIntensity: 0.2
          });
        case 'Venus':
          return new THREE.MeshStandardMaterial({
            color: 0xE7C697,
            metalness: 0.3,
            roughness: 0.6,
            emissive: 0xE7C697,
            emissiveIntensity: 0.2
          });
        case 'Earth':
          return new THREE.MeshStandardMaterial({
            color: 0x4169E1,
            metalness: 0.3,
            roughness: 0.4,
            emissive: 0x4169E1,
            emissiveIntensity: 0.2
          });
        case 'Mars':
          return new THREE.MeshStandardMaterial({
            color: 0xA0522D,
            metalness: 0.3,
            roughness: 0.5,
            emissive: 0xA0522D,
            emissiveIntensity: 0.2
          });
        case 'Jupiter':
          return new THREE.MeshStandardMaterial({
            color: 0xDAA520,
            metalness: 0.3,
            roughness: 0.4,
            emissive: 0xDAA520,
            emissiveIntensity: 0.2
          });
        case 'Saturn':
          return new THREE.MeshStandardMaterial({
            color: 0xCD853F,
            metalness: 0.4,
            roughness: 0.4,
            emissive: 0xCD853F,
            emissiveIntensity: 0.2
          });
        case 'Uranus':
          return new THREE.MeshStandardMaterial({
            color: 0x4682B4,
            metalness: 0.5,
            roughness: 0.3,
            emissive: 0x4682B4,
            emissiveIntensity: 0.2
          });
        case 'Neptune':
          return new THREE.MeshStandardMaterial({
            color: 0x4169E1,
            metalness: 0.5,
            roughness: 0.3,
            emissive: 0x4169E1,
            emissiveIntensity: 0.2
          });
        default:
          return new THREE.MeshStandardMaterial({
            color: 0x808080
          });
      }
    },

    init() {
      if (isInitialized) return;
      
      scene = new THREE.Scene();
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

      // 调整相机参数
      camera = new THREE.PerspectiveCamera(
        75,
        window.innerWidth / window.innerHeight,
        0.001,
        10000
      );
      camera.position.set(0, 200, 200);
      camera.lookAt(0, 0, 0);

      renderer = new THREE.WebGLRenderer({
        antialias: true,
        powerPreference: "high-performance",
        alpha: true
      });
      renderer.setSize(window.innerWidth, window.innerHeight);
      renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
      renderer.shadowMap.enabled = true;
      renderer.shadowMap.type = THREE.PCFSoftShadowMap;
      renderer.toneMapping = THREE.ACESFilmicToneMapping;
      renderer.toneMappingExposure = 1.5;  // 增加曝光
      
      const container = document.getElementById('canvas-container');
      if (container) {
        container.appendChild(renderer.domElement);
      }

      // 调整控制器参数
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

      // 添加辅助光源
      const hemisphereLight = new THREE.HemisphereLight(0xffffbb, 0x080820, 1);
      scene.add(hemisphereLight);

      window.addEventListener('resize', this.onWindowResize, false);
      
      isInitialized = true;
      this.render();
    },

    render() {
      if (!isInitialized) return;
      
      if (controls) {
        controls.update();
      }

      if (renderer && scene && camera) {
        renderer.render(scene, camera);
      }

      requestAnimationFrame(this.render);
    },

    onWindowResize() {
      if (!isInitialized) return;
      
      this.width = window.innerWidth;
      this.height = window.innerHeight;
      
      if (camera) {
        camera.aspect = this.width / this.height;
        camera.updateProjectionMatrix();
      }
      
      if (renderer) {
        renderer.setSize(this.width, this.height);
      }
    },

    async exportConfig() {
      try {
        const response = await axios.get(
          `http://localhost:8081/api/export-config${this.algorithm === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`
        );
        
        // 创建下载
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
        console.error('Error exporting config:', error);
      }
    },

    importConfig() {
      this.$refs.fileInput.click();
    },

    async handleFileUpload(event) {
      const file = event.target.files[0];
      if (!file) return;

      try {
        const reader = new FileReader();
        reader.onload = async (e) => {
          const config = JSON.parse(e.target.result);
          
          // 发送配置到服务器
          await axios.post(
            `http://localhost:8081/api/import-config${this.algorithm === 'barnes-hut' ? '?algorithm=barnes-hut' : ''}`,
            config
          );
          
          // 重新获取系统状态并更新场景
          await this.fetchInitialState();

          // 清除现有的天体
          celestialBodies.forEach((body) => {
            scene.remove(body);
          });
          celestialBodies.clear();

          // 重新创建天体
          if (config.bodies) {
            this.updateCelestialBodies(config.bodies);
          }

          // 重置相机位置以便查看新的配置
          if (camera) {
            camera.position.set(0, 200, 200);
            camera.lookAt(0, 0, 0);
          }
        };
        reader.readAsText(file);
      } catch (error) {
        console.error('Error importing config:', error);
      } finally {
        // 清除文件输入，允许重复上传同一个文件
        event.target.value = '';
      }
    }
  },
  beforeUnmount() {
    if (this.simulationInterval) {
      clearInterval(this.simulationInterval);
    }

    isInitialized = false;
    window.removeEventListener('resize', this.onWindowResize);

    // 清理资源
    if (renderer) {
      renderer.dispose();
      renderer.domElement.remove();
    }

    if (controls) {
      controls.dispose();
    }

    celestialBodies.clear();
    scene = null;
    camera = null;
    renderer = null;
    controls = null;
  }
};
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
  position: absolute;
  top: 20px;
  right: 20px;
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
</style>
