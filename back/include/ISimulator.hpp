#pragma once

#include "CelestialBody.hpp"
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace GEngine {

class ISimulator {
public:
    virtual ~ISimulator() = default;

    // 基本操作
    virtual void addBody(std::shared_ptr<CelestialBody> body) = 0;
    virtual void removeBody(const std::string& name) = 0;
    virtual void clear() = 0;
    
    // 模拟控制
    virtual void step() = 0;
    virtual void reset() = 0;
    
    // 状态访问
    virtual nlohmann::json getSystemState() const = 0;
    virtual std::vector<std::shared_ptr<CelestialBody>> getBodies() const = 0;
    
    // 配置
    virtual void configure(const nlohmann::json& config) = 0;
};

} // namespace GEngine 