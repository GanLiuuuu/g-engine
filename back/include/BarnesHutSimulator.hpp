#pragma once

#include "ISimulator.hpp"
#include "OctreeNode.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace GEngine {

class BarnesHutSimulator : public ISimulator {
private:
    std::vector<std::shared_ptr<CelestialBody>> bodies_;
    std::unique_ptr<OctreeNode> root_;

    void buildOctree();

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

    // 实现引力场计算（使用Barnes-Hut算法）
    Vector3D calculateGravitationalField(const Vector3D& position) const override {
        if (!root_) {
            const_cast<BarnesHutSimulator*>(this)->buildOctree();
        }

        Vector3D totalField(0, 0, 0);
        const auto& config = SimulationConfig::getInstance();

        // 使用Barnes-Hut树结构计算引力场
        std::function<void(const OctreeNode*, const Vector3D&)> calculateField = 
            [&](const OctreeNode* node, const Vector3D& pos) {
                if (!node || (node->getTotalMass() < 1e-10)) return;

                Vector3D r = pos - node->getCenterOfMass();
                double distance = r.magnitude();

                // 如果节点足够远，使用其质心近似
                if (node->getSize() / distance < config.barnesHutTheta) {
                    if (distance > 0) {
                        double fieldMagnitude = config.gravityConstant * node->getTotalMass() / 
                                             (distance * distance);
                        totalField = totalField + r.normalize() * (-fieldMagnitude);
                    }
                }
                // 否则递归计算子节点
                else {
                    for (int i = 0; i < 8; ++i) {
                        if (const auto& child = node->getChild(i)) {
                            calculateField(child.get(), pos);
                        }
                    }
                }
            };

        calculateField(root_.get(), position);
        return totalField;
    }
};

} // namespace GEngine 