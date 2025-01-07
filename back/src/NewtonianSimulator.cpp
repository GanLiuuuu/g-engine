#include "../include/NewtonianSimulator.hpp"
#include "../include/Config.hpp"
#include <algorithm>
#include <iostream>

namespace GEngine {

void NewtonianSimulator::addBody(std::shared_ptr<CelestialBody> body) {
    bodies_.push_back(body);
}

void NewtonianSimulator::removeBody(const std::string& name) {
    bodies_.erase(
        std::remove_if(bodies_.begin(), bodies_.end(),
            [&name](const auto& body) { return body->getName() == name; }),
        bodies_.end()
    );
}

void NewtonianSimulator::clear() {
    bodies_.clear();
}

void NewtonianSimulator::step() {
    const auto& config = SimulationConfig::getInstance();
    
    #pragma omp parallel for
    for (size_t i = 0; i < bodies_.size(); ++i) {
        Vector3D totalForce(0, 0, 0);
        for (size_t j = 0; j < bodies_.size(); ++j) {
            if (i != j) {
                Vector3D r = bodies_[j]->getPosition() - bodies_[i]->getPosition();
                double distance = r.magnitude();
                
                if (distance > (bodies_[i]->getRadius() + bodies_[j]->getRadius())) {
                    double forceMagnitude = config.gravityConstant * 
                                          bodies_[i]->getMass() * 
                                          bodies_[j]->getMass() / 
                                          (distance * distance);
                    totalForce = totalForce + r.normalize() * forceMagnitude;
                }
            }
        }
        bodies_[i]->setAcceleration(totalForce * (1.0 / bodies_[i]->getMass()));
    }

    #pragma omp parallel for
    for (size_t i = 0; i < bodies_.size(); ++i) {
        bodies_[i]->updateState(config.timeStep);
    }

    // 3. 碰撞检测
    detectCollisions();
}

void NewtonianSimulator::reset() {
    for (auto& body : bodies_) {
        body->setAcceleration(Vector3D(0, 0, 0));
        body->setVelocity(Vector3D(0, 0, 0));
    }
}

nlohmann::json NewtonianSimulator::getSystemState() const {
    nlohmann::json state = nlohmann::json::array();
    for (const auto& body : bodies_) {
        state.push_back(body->toJson());
    }
    return state;
}

std::vector<std::shared_ptr<CelestialBody>> NewtonianSimulator::getBodies() const {
    return bodies_;
}

void NewtonianSimulator::configure(const nlohmann::json& config) {
    SimulationConfig::getInstance().loadFromJson(config);
}

void NewtonianSimulator::detectCollisions() {
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
                
                std::cout << "发生碰撞！！！！！！！！！！！！！！！！！！！！" << std::endl;
            }
        }
    }
}

}