#pragma once

#include "ISimulator.hpp"
#include <vector>

namespace GEngine {

class NewtonianSimulator : public ISimulator {
private:
    std::vector<std::shared_ptr<CelestialBody>> bodies_;

public:
    // 基本操作
    void addBody(std::shared_ptr<CelestialBody> body) override;
    void removeBody(const std::string& name) override;
    void clear() override;
    
    // 模拟控制
    void step() override;
    void reset() override;
    
    // 状态访问
    nlohmann::json getSystemState() const override;
    std::vector<std::shared_ptr<CelestialBody>> getBodies() const override;
    
    // 配置
    void configure(const nlohmann::json& config) override;
};

} // namespace GEngine 