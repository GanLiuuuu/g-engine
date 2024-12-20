<template>
  <div class="universe">
    <div id="canvas-container"></div>
    <button class="screenshot-btn" @click="takeScreenshot">截图</button>
  </div>
</template>

<script>
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';

// 声明全局变量，避免响应式
let scene = null;
let camera = null;
let renderer = null;
let controls = null;
let isInitialized = false;

export default {
  name: 'App',
  data() {
    return {
      width: window.innerWidth,
      height: window.innerHeight
    };
  },
  mounted() {
    this.init();
  },
  methods: {
    init() {
      if (isInitialized) return;
      
      // Scene setup with gradient background
      scene = new THREE.Scene();
      const bgColor1 = new THREE.Color(0x000020); // 深蓝色
      const bgColor2 = new THREE.Color(0x110024); // 深紫色
      const canvas = document.createElement('canvas');
      const context = canvas.getContext('2d');
      canvas.width = 2;
      canvas.height = 2;
      
      // 创建渐变背景
      const gradient = context.createLinearGradient(0, 0, 0, 2);
      gradient.addColorStop(0, `#${bgColor1.getHexString()}`);
      gradient.addColorStop(1, `#${bgColor2.getHexString()}`);
      
      context.fillStyle = gradient;
      context.fillRect(0, 0, 2, 2);
      
      const bgTexture = new THREE.CanvasTexture(canvas);
      scene.background = bgTexture;

      // Camera setup
      camera = new THREE.PerspectiveCamera(
        75,
        window.innerWidth / window.innerHeight,
        0.1,
        1000
      );
      camera.position.set(0, 30, 50);
      camera.lookAt(0, 0, 0);

      // Renderer setup with better quality
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
      renderer.toneMappingExposure = 1;
      
      const container = document.getElementById('canvas-container');
      if (container) {
        container.appendChild(renderer.domElement);
      }

      // Controls
      controls = new OrbitControls(camera, renderer.domElement);
      controls.enableDamping = true;
      controls.dampingFactor = 0.05;
      controls.screenSpacePanning = false;
      controls.minDistance = 20;
      controls.maxDistance = 100;

      // Enhanced lighting
      const ambientLight = new THREE.AmbientLight(0x404040, 0.5);
      scene.add(ambientLight);

      // Create detailed solar system
      this.createSolarSystem();

      // Handle resize
      window.addEventListener('resize', this.onWindowResize, false);
      
      isInitialized = true;
      
      // Initial render
      this.render();
    },

    createSolarSystem() {
      // 创建太阳
      const sunRadius = 5;
      const sunGeometry = new THREE.SphereGeometry(sunRadius, 64, 64);
      const sunMaterial = new THREE.MeshBasicMaterial({
        color: 0xffff00,
        emissive: 0xffaa00,
        emissiveIntensity: 1.5
      });
      const sun = new THREE.Mesh(sunGeometry, sunMaterial);
      scene.add(sun);

      // 添加太阳光源系统
      const sunLight = new THREE.PointLight(0xffffff, 5, 300);
      sunLight.position.set(0, 0, 0);
      sunLight.castShadow = true;
      sunLight.shadow.mapSize.width = 2048;
      sunLight.shadow.mapSize.height = 2048;
      sunLight.shadow.camera.near = 0.1;
      sunLight.shadow.camera.far = 300;
      sun.add(sunLight);

      // 添加第二个光源来增强照明效果
      const secondaryLight = new THREE.PointLight(0xffaa00, 3, 150);
      secondaryLight.position.set(0, 0, 0);
      sun.add(secondaryLight);

      // 添加第三个光源用于背光照明
      const backLight = new THREE.PointLight(0xffddaa, 2, 100);
      backLight.position.set(0, 20, 0);
      scene.add(backLight);

      // 创建行星
      const planetData = [
        { name: 'Mercury', radius: 0.8, distance: 10, color: 0x887788, metalness: 0.3, roughness: 0.4 },
        { name: 'Venus', radius: 1.2, distance: 15, color: 0xE7C697, metalness: 0.3, roughness: 0.5 },
        { name: 'Earth', radius: 1.5, distance: 20, color: 0x4169E1, metalness: 0.3, roughness: 0.4 },
        { name: 'Mars', radius: 1.0, distance: 25, color: 0xA0522D, metalness: 0.3, roughness: 0.5 },
        { name: 'Jupiter', radius: 3.0, distance: 35, color: 0xDAA520, metalness: 0.4, roughness: 0.4 },
        { name: 'Saturn', radius: 2.5, distance: 45, color: 0xCD853F, metalness: 0.4, roughness: 0.4 },
        { name: 'Uranus', radius: 1.8, distance: 55, color: 0x4682B4, metalness: 0.5, roughness: 0.3 },
        { name: 'Neptune', radius: 1.8, distance: 65, color: 0x4169E1, metalness: 0.5, roughness: 0.3 }
      ];

      planetData.forEach((data, index) => {
        const geometry = new THREE.SphereGeometry(data.radius, 32, 32);
        const material = new THREE.MeshStandardMaterial({
          color: data.color,
          metalness: data.metalness,
          roughness: data.roughness,
          emissive: data.color,
          emissiveIntensity: 0.05,
          envMapIntensity: 1.5
        });
        
        const planet = new THREE.Mesh(geometry, material);
        const angle = (index / planetData.length) * Math.PI * 2;
        planet.position.x = Math.cos(angle) * data.distance;
        planet.position.z = Math.sin(angle) * data.distance;
        planet.castShadow = true;
        planet.receiveShadow = true;
        
        // 添加大气层效果
        if (['Earth', 'Venus', 'Mars', 'Jupiter', 'Saturn', 'Uranus', 'Neptune'].includes(data.name)) {
          const atmosphereGeometry = new THREE.SphereGeometry(data.radius * 1.05, 32, 32);
          const atmosphereMaterial = new THREE.MeshPhongMaterial({
            color: data.color,
            transparent: true,
            opacity: 0.3,
            side: THREE.BackSide,
            emissive: data.color,
            emissiveIntensity: 0.2
          });
          const atmosphere = new THREE.Mesh(atmosphereGeometry, atmosphereMaterial);
          planet.add(atmosphere);
        }
        
        scene.add(planet);

        // 添加轨道
        const orbitGeometry = new THREE.RingGeometry(data.distance - 0.1, data.distance + 0.1, 90);
        const orbitMaterial = new THREE.MeshBasicMaterial({
          color: 0x666666,
          side: THREE.DoubleSide,
          transparent: true,
          opacity: 0.1
        });
        const orbit = new THREE.Mesh(orbitGeometry, orbitMaterial);
        orbit.rotation.x = Math.PI / 2;
        scene.add(orbit);
      });

      // 添加星空背景，调整星星的大小和数量
      const starGeometry = new THREE.BufferGeometry();
      const starVertices = [];
      const starSizes = [];
      const starColors = [];
      
      for (let i = 0; i < 15000; i++) {
        const x = (Math.random() - 0.5) * 1000;
        const y = (Math.random() - 0.5) * 1000;
        const z = (Math.random() - 0.5) * 1000;
        starVertices.push(x, y, z);

        // 随机星星大小
        const size = Math.random() * 0.5 + 0.1;
        starSizes.push(size);

        // 随机星星颜色
        const color = new THREE.Color();
        if (Math.random() < 0.3) {
          // 30% 概率是淡蓝色星星
          color.setHSL(0.6, 0.8, 0.9);
        } else if (Math.random() < 0.6) {
          // 30% 概率是淡黄色星星
          color.setHSL(0.15, 0.6, 0.9);
        } else {
          // 40% 概率是白色星星
          color.setHSL(0, 0, 1);
        }
        starColors.push(color.r, color.g, color.b);
      }

      starGeometry.setAttribute('position', new THREE.Float32BufferAttribute(starVertices, 3));
      starGeometry.setAttribute('size', new THREE.Float32BufferAttribute(starSizes, 1));
      starGeometry.setAttribute('color', new THREE.Float32BufferAttribute(starColors, 3));

      const starMaterial = new THREE.PointsMaterial({
        size: 0.5,
        transparent: true,
        opacity: 0.8,
        vertexColors: true,
        sizeAttenuation: true,
        blending: THREE.AdditiveBlending
      });

      const starField = new THREE.Points(starGeometry, starMaterial);
      scene.add(starField);

      // 添加星云效果
      const nebulaMaterial = new THREE.ShaderMaterial({
        uniforms: {
          time: { value: 0 }
        },
        vertexShader: `
          varying vec2 vUv;
          void main() {
            vUv = uv;
            gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
          }
        `,
        fragmentShader: `
          uniform float time;
          varying vec2 vUv;
          
          float random(vec2 st) {
            return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
          }
          
          void main() {
            vec2 st = vUv;
            float r = random(st + time * 0.1);
            vec3 color = vec3(0.2, 0.1, 0.3) * r;
            gl_FragColor = vec4(color, 0.1);
          }
        `,
        transparent: true,
        blending: THREE.AdditiveBlending
      });

      const nebulaGeometry = new THREE.PlaneGeometry(1000, 1000);
      const nebula = new THREE.Mesh(nebulaGeometry, nebulaMaterial);
      nebula.position.z = -500;
      scene.add(nebula);
    },

    render() {
      if (!isInitialized) return;
      
      if (controls) {
        controls.update();
      }

      // 更新星云动画
      const nebula = scene.children.find(child => child.material && child.material.type === 'ShaderMaterial');
      if (nebula) {
        nebula.material.uniforms.time.value += 0.001;
      }

      if (renderer && scene && camera) {
        renderer.render(scene, camera);
      }

      requestAnimationFrame(this.render);
    },

    async takeScreenshot() {
      if (!isInitialized) return;

      try {
        const imgData = renderer.domElement.toDataURL('image/png');
        const link = document.createElement('a');
        link.href = imgData;
        link.download = `solar-system-${new Date().getTime()}.png`;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
      } catch (error) {
        console.error('Screenshot failed:', error);
      }
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
    }
  },
  beforeUnmount() {
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

.screenshot-btn {
  position: absolute;
  top: 20px;
  right: 20px;
  padding: 10px 20px;
  background-color: rgba(255, 255, 255, 0.2);
  border: 1px solid rgba(255, 255, 255, 0.3);
  color: white;
  border-radius: 4px;
  cursor: pointer;
  transition: background-color 0.3s;
}

.screenshot-btn:hover {
  background-color: rgba(255, 255, 255, 0.3);
}
</style>
