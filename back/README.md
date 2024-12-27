# G-Engine

G-Engine 是一个高性能的天体物理模拟引擎，支持经典牛顿万有引力和 Barnes-Hut 算法。

## 特性

- 支持多种模拟算法：
  - 经典牛顿万有引力算法 (O(n²))
  - Barnes-Hut 算法 (O(n log n))
- 高性能实现：
  - OpenMP 并行计算
  - 八叉树空间划分
  - 自适应精度控制
- 模块化设计：
  - 清晰的 API 接口
  - 可扩展的架构
  - JSON 配置支持

## 构建要求

- C++17 兼容的编译器
- CMake 3.10 或更高版本
- OpenMP
- nlohmann_json

### 在 macOS 上安装依赖

```bash
brew install libomp nlohmann-json
```

### 在 Ubuntu/Debian 上安装依赖

```bash
sudo apt-get install libomp-dev nlohmann-json3-dev
```

## 构建说明

```bash
mkdir build && cd build
cmake ..
make
```

## 使用示例

```cpp
#include <gengine/ISimulator.hpp>
#include <gengine/NewtonianSimulator.hpp>

// 创建模拟器实例
auto simulator = std::make_shared<GEngine::NewtonianSimulator>();

// 添加天体
simulator->addBody(std::make_shared<GEngine::CelestialBody>(
    "Earth", 5.972e24, 6371000,
    GEngine::Vector3D(0, 0, 0),
    GEngine::Vector3D(0, 29.78e3, 0)
));

// 运行模拟
simulator->step();
```

## 许可证

MIT License 