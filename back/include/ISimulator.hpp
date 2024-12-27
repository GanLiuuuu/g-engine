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

    // 引力场计算
    virtual Vector3D calculateGravitationalField(const Vector3D& position) const = 0;
    
    // 获取引力场数据
    virtual nlohmann::json getGravitationalFieldData(
        const Vector3D& center,
        double size,
        int resolution
    ) const {
        nlohmann::json fieldData = nlohmann::json::array();
        double step = size / resolution;
        
        for (int x = 0; x < resolution; ++x) {
            for (int y = 0; y < resolution; ++y) {
                for (int z = 0; z < resolution; ++z) {
                    Vector3D pos(
                        center.x() + (x - resolution/2) * step,
                        center.y() + (y - resolution/2) * step,
                        center.z() + (z - resolution/2) * step
                    );
                    
                    Vector3D field = calculateGravitationalField(pos);
                    double magnitude = field.magnitude();
                    
                    if (magnitude > 1e-10) {  // 只记录有意义的数据点
                        fieldData.push_back({
                            {"position", pos.toJson()},
                            {"field", field.toJson()},
                            {"magnitude", magnitude}
                        });
                    }
                }
            }
        }
        
        return fieldData;
    }
};

} // namespace GEngine 