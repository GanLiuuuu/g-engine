# Solar System Simulation (G-Engine)

这是一个基于物理引擎的太阳系模拟器，使用C++后端进行物理计算，Vue.js前端进行3D可视化展示。

## 功能特点

- 精确的物理模拟：使用牛顿万有引力定律
- 支持两种模拟算法：
  - 经典N体模拟
  - Barnes-Hut算法（优化的N体模拟）
- 包含太阳系主要天体：太阳、水星、金星、地球、火星、木星、土星、天王星、海王星
- 实时3D可视化
- 可调节的时间步长

## 依赖要求

### 后端依赖
- C++17或更高版本
- CMake 3.10或更高版本
- nlohmann/json库（用于JSON处理）
- OpenMP（用于并行计算）

### 前端依赖
- Node.js 14.0或更高版本
- npm或yarn包管理器
- Vue.js 3.x
- Three.js（用于3D渲染）

## 安装步骤

1. 克隆仓库：
```bash
git clone [repository-url]
cd g-engine
```

2. 安装后端依赖（MacOS）：
```bash
brew install nlohmann-json
brew install libomp
```

3. 编译后端：
```bash
cd back
mkdir build && cd build
cmake ..
make
```

4. 安装前端依赖：
```bash
cd ../../front
npm install
```

## 运行项目

1. 启动后端服务器：
```bash
cd back/build
./server
```

2. 在新的终端窗口中启动前端开发服务器：
```bash
cd front
npm run serve
```

3. 在浏览器中访问：
```
http://localhost:8080
```

## 使用说明

- 使用鼠标拖动可以旋转视角
- 使用鼠标滚轮可以缩放
- 右键拖动可以平移视角
- 界面上方显示了每个天体的实时状态信息

## 技术实现

- 后端使用C++实现物理计算，包括：
  - 牛顿万有引力计算
  - Verlet积分方法用于提高精度
  - Barnes-Hut算法优化N体问题
  - OpenMP并行计算加速
  
- 前端使用Vue.js和Three.js实现3D可视化，包括：
  - 实时3D渲染
  - 相机控制
  - 天体缩放和材质
  - WebSocket实时数据更新

## 注意事项

- 模拟使用了真实的天文数据，但进行了一些简化：
  - 轨道假设为圆形（实际为椭圆）
  - 忽略了轨道倾角
  - 仅考虑主要天体的引力作用
- 为了可视化效果，天体的大小进行了适当缩放，不代表真实比例
