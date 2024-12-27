#pragma once

#include "Vector3D.hpp"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace GEngine {

class CelestialBody {
protected:
    std::string name_;
    double mass_;
    double radius_;
    Vector3D position_;
    Vector3D velocity_;
    Vector3D acceleration_;

public:
    CelestialBody(const std::string& name, double mass, double radius,
                 const Vector3D& position, const Vector3D& velocity)
        : name_(name), mass_(mass), radius_(radius), 
          position_(position), velocity_(velocity), acceleration_(0, 0, 0) {}

    virtual ~CelestialBody() = default;

    // Getters
    const std::string& getName() const { return name_; }
    double getMass() const { return mass_; }
    double getRadius() const { return radius_; }
    const Vector3D& getPosition() const { return position_; }
    const Vector3D& getVelocity() const { return velocity_; }
    const Vector3D& getAcceleration() const { return acceleration_; }

    // Setters
    void setPosition(const Vector3D& pos) { position_ = pos; }
    void setVelocity(const Vector3D& vel) { velocity_ = vel; }
    void setAcceleration(const Vector3D& acc) { acceleration_ = acc; }

    // 更新状态
    virtual void updateState(double dt) {
        Vector3D old_position = position_;
        Vector3D half_velocity = velocity_ + acceleration_ * (dt * 0.5);
        position_ = position_ + half_velocity * dt;
        velocity_ = half_velocity + acceleration_ * (dt * 0.5);
    }

    // JSON序列化
    virtual nlohmann::json toJson() const {
        return {
            {"name", name_},
            {"mass", mass_},
            {"radius", radius_},
            {"position", position_.toJson()},
            {"velocity", velocity_.toJson()},
            {"acceleration", acceleration_.toJson()}
        };
    }

    static std::shared_ptr<CelestialBody> fromJson(const nlohmann::json& j) {
        return std::make_shared<CelestialBody>(
            j["name"].get<std::string>(),
            j["mass"].get<double>(),
            j["radius"].get<double>(),
            Vector3D::fromJson(j["position"]),
            Vector3D::fromJson(j["velocity"])
        );
    }
};

} // namespace GEngine 