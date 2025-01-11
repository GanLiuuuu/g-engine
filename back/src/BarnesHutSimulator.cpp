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
    for (size_t i = 0; i < bodies_.size(); ++i) {
        Vector3D force = root_->calculateForce(*bodies_[i]);
        bodies_[i]->setAcceleration(force * (1.0 / bodies_[i]->getMass()));
    }

    for (size_t i = 0; i < bodies_.size(); ++i) {
        bodies_[i]->updateState(config.timeStep);
    }

    // 3. 碰撞检测
    detectCollisions();
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

void BarnesHutSimulator::detectCollisions() {
    for (size_t i = 0; i < bodies_.size(); ++i) {
        for (size_t j = i + 1; j < bodies_.size(); ++j) {
            auto bodyA = bodies_[i];
            auto bodyB = bodies_[j];

            Vector3D diff = bodyA->getPosition() - bodyB->getPosition();
            double distance = diff.magnitude();
            double collisionDist = bodyA->getRadius() + bodyB->getRadius();

            if (distance < collisionDist) {
                nlohmann::json collisionEvent;
                collisionEvent["type"] = "collision";
                collisionEvent["time"] = 0; 
                collisionEvent["bodies"] = { bodyA->getName(), bodyB->getName() };
                collisionEvent["distance"] = distance;
                collisionEvent["message"] =
                    "Collision occurred between " + bodyA->getName() + " and " + bodyB->getName();

                eventLog.push_back(collisionEvent);

            }
        }
    }
}

} // namespace GEngine 