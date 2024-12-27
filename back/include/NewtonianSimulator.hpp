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
};

} 