#include "../include/BarnesHutSimulator.hpp"
#include "../include/Config.hpp"
#include <algorithm>

namespace GEngine {

void BarnesHutSimulator::addBody(std::shared_ptr<CelestialBody> body) {
    bodies_.push_back(body);
}

void BarnesHutSimulator::removeBody(const std::string& name) {
    bodies_.erase(
        std::remove_if(bodies_.begin(), bodies_.end(),
            [&name](const auto& body) { return body->getName() == name; }),
        bodies_.end()
    );
}

void BarnesHutSimulator::clear() {
    bodies_.clear();
}

void BarnesHutSimulator::buildOctree() {
    const auto& config = SimulationConfig::getInstance();
    root_ = std::make_unique<OctreeNode>(Vector3D(0, 0, 0), config.universeSize);
    for (const auto& body : bodies_) {
        root_->insert(body);
    }
}

void BarnesHutSimulator::step() {
    buildOctree();
    const auto& config = SimulationConfig::getInstance();

    #pragma omp parallel for
    for (size_t i = 0; i < bodies_.size(); ++i) {
        Vector3D force = root_->calculateForce(*bodies_[i]);
        bodies_[i]->setAcceleration(force * (1.0 / bodies_[i]->getMass()));
    }

    #pragma omp parallel for
    for (size_t i = 0; i < bodies_.size(); ++i) {
        bodies_[i]->updateState(config.timeStep);
    }
}

void BarnesHutSimulator::reset() {
    for (auto& body : bodies_) {
        body->setAcceleration(Vector3D(0, 0, 0));
        body->setVelocity(Vector3D(0, 0, 0));
    }
    root_.reset();
}

nlohmann::json BarnesHutSimulator::getSystemState() const {
    nlohmann::json state = nlohmann::json::array();
    for (const auto& body : bodies_) {
        state.push_back(body->toJson());
    }
    return state;
}

std::vector<std::shared_ptr<CelestialBody>> BarnesHutSimulator::getBodies() const {
    return bodies_;
}

void BarnesHutSimulator::configure(const nlohmann::json& config) {
    SimulationConfig::getInstance().loadFromJson(config);
}

} // namespace GEngine 