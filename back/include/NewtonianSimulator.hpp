#pragma once

#include "ISimulator.hpp"
#include <vector>

namespace GEngine {

class NewtonianSimulator : public ISimulator {
private:
    std::vector<std::shared_ptr<CelestialBody>> bodies_;

public:
    void addBody(std::shared_ptr<CelestialBody> body) override;
    void removeBody(const std::string& name) override;
    void clear() override;
    
    void step() override;
    void reset() override;
    
    nlohmann::json getSystemState() const override;
    std::vector<std::shared_ptr<CelestialBody>> getBodies() const override;
    
    void configure(const nlohmann::json& config) override;

    // 实现引力场计算
    Vector3D calculateGravitationalField(const Vector3D& position) const override {
        Vector3D totalField(0, 0, 0);
        const auto& config = SimulationConfig::getInstance();

        for (const auto& body : bodies_) {
            Vector3D r = position - body->getPosition();
            double distance = r.magnitude();
            
            if (distance > body->getRadius()) {  // 避免在天体内部计算
                double fieldMagnitude = config.gravityConstant * body->getMass() / 
                                     (distance * distance);
                totalField = totalField + r.normalize() * (-fieldMagnitude);  // 引力场方向指向质量中心
            }
        }
        
        return totalField;
    }
};

} 