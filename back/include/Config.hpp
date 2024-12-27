#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace GEngine {

class SimulationConfig {
public:
    static SimulationConfig& getInstance() {
        static SimulationConfig instance;
        return instance;
    }

    // 模拟参数
    double timeStep = 864000.0;  // 默认时间步长（秒）
    double gravityConstant = 6.67430e-11;  // 万有引力常数
    double barnesHutTheta = 0.5;  // Barnes-Hut算法的精度参数
    double universeSize = 1e12;   // 宇宙大小（米）
    bool timeDirectionForward = true;  // 时间方向（true为正向，false为逆向）

    // 从JSON加载配置
    void loadFromJson(const nlohmann::json& config) {
        if (config.contains("timeStep")) timeStep = config["timeStep"];
        if (config.contains("gravityConstant")) gravityConstant = config["gravityConstant"];
        if (config.contains("barnesHutTheta")) barnesHutTheta = config["barnesHutTheta"];
        if (config.contains("universeSize")) universeSize = config["universeSize"];
        if (config.contains("timeDirectionForward")) timeDirectionForward = config["timeDirectionForward"];
    }

    // 导出为JSON
    nlohmann::json toJson() const {
        return {
            {"timeStep", timeStep},
            {"gravityConstant", gravityConstant},
            {"barnesHutTheta", barnesHutTheta},
            {"universeSize", universeSize},
            {"timeDirectionForward", timeDirectionForward}
        };
    }

private:
    SimulationConfig() = default;
    SimulationConfig(const SimulationConfig&) = delete;
    SimulationConfig& operator=(const SimulationConfig&) = delete;
};

} // namespace GEngine 