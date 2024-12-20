#include "./httplib.h"
#include <vector>
#include <memory>
#include <cmath>
#include <string>
#include <nlohmann/json.hpp>
#include <omp.h>
#include <list>
#include <array>
#include <iostream>

using json = nlohmann::json;

namespace Constants {
    const double G = 6.67430e-11; 
    const double TIME_STEP = 864000.0;   //10天
    const double THETA = 0.5;      
}


class Vector3D {
private:
    double x_, y_, z_;

public:
    Vector3D(double x = 0, double y = 0, double z = 0) : x_(x), y_(y), z_(z) {}

    double x() const { return x_; }
    double y() const { return y_; }
    double z() const { return z_; }

    Vector3D operator+(const Vector3D& other) const {
        return Vector3D(x_ + other.x_, y_ + other.y_, z_ + other.z_);
    }

    Vector3D operator-(const Vector3D& other) const {
        return Vector3D(x_ - other.x_, y_ - other.y_, z_ - other.z_);
    }

    Vector3D operator*(double scalar) const {
        return Vector3D(x_ * scalar, y_ * scalar, z_ * scalar);
    }

    double magnitude() const {
        return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_);
    }

    Vector3D normalize() const {
        double mag = magnitude();
        if (mag > 0) {
            return Vector3D(x_ / mag, y_ / mag, z_ / mag);
        }
        return *this;
    }
};

class CelestialBody {
protected:
    std::string name_;
    double mass_;          // 质量（kg）
    double radius_;        // 半径（m）
    Vector3D position_;    // 位置
    Vector3D velocity_;    // 速度
    Vector3D acceleration_;// 加速度

    friend class NewtonianSimulator;
    friend class BarnesHutSimulator;

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

    virtual void updateState(double dt) {
        Vector3D old_position = position_;
        Vector3D half_velocity = velocity_ + acceleration_ * (dt * 0.5);
        position_ = position_ + half_velocity * dt;
        velocity_ = half_velocity + acceleration_ * (dt * 0.5);
    }

    virtual json toJson() const {
        return {
            {"name", name_},
            {"mass", mass_},
            {"radius", radius_},
            {"position", {position_.x(), position_.y(), position_.z()}},
            {"velocity", {velocity_.x(), velocity_.y(), velocity_.z()}},
            {"acceleration", {acceleration_.x(), acceleration_.y(), acceleration_.z()}}
        };
    }
};

class OctreeNode {
private:
    Vector3D center_;
    double size_;
    double totalMass_;
    Vector3D centerOfMass_;
    std::array<std::unique_ptr<OctreeNode>, 8> children_;
    std::list<std::shared_ptr<CelestialBody>> bodies_;

public:
    OctreeNode(const Vector3D& center, double size)
        : center_(center), size_(size), totalMass_(0) {}

    void insert(std::shared_ptr<CelestialBody> body) {
        if (bodies_.empty() && !children_[0]) {
            bodies_.push_back(body);
            totalMass_ = body->getMass();
            centerOfMass_ = body->getPosition();
            return;
        }

        if (!children_[0]) {
            subdivide();
        }

        int octant = getOctant(body->getPosition());
        children_[octant]->insert(body);

        totalMass_ += body->getMass();
        centerOfMass_ = centerOfMass_ * (totalMass_ - body->getMass()) + 
                       body->getPosition() * body->getMass();
        centerOfMass_ = centerOfMass_ * (1.0 / totalMass_);
    }

    Vector3D calculateForce(const CelestialBody& body) const {
        if (bodies_.empty() && !children_[0]) {
            return Vector3D(0, 0, 0);
        }

        double distance = (centerOfMass_ - body.getPosition()).magnitude();
        
        // 如果节点足够远，使用质心近似
        if (size_ / distance < Constants::THETA) {
            Vector3D r = centerOfMass_ - body.getPosition();
            double distanceCube = distance * distance * distance;
            return r * (Constants::G * totalMass_ * body.getMass() / distanceCube);
        }

        // 否则递归计算子节点
        Vector3D totalForce(0, 0, 0);
        for (const auto& child : children_) {
            if (child) {
                totalForce = totalForce + child->calculateForce(body);
            }
        }
        return totalForce;
    }

private:
    void subdivide() {
        double halfSize = size_ / 2;
        for (int i = 0; i < 8; ++i) {
            double x = center_.x() + ((i & 1) ? halfSize : -halfSize);
            double y = center_.y() + ((i & 2) ? halfSize : -halfSize);
            double z = center_.z() + ((i & 4) ? halfSize : -halfSize);
            children_[i] = std::make_unique<OctreeNode>(Vector3D(x, y, z), halfSize);
        }

        for (auto& body : bodies_) {
            int octant = getOctant(body->getPosition());
            children_[octant]->insert(body);
        }
        bodies_.clear();
    }

    int getOctant(const Vector3D& position) const {
        int octant = 0;
        if (position.x() > center_.x()) octant |= 1;
        if (position.y() > center_.y()) octant |= 2;
        if (position.z() > center_.z()) octant |= 4;
        return octant;
    }
};

class BarnesHutSimulator {
private:
    std::vector<std::shared_ptr<CelestialBody>> bodies_;
    double universeSize_;

public:
    BarnesHutSimulator(double universeSize = 1e12) : universeSize_(universeSize) {}

    void clear() {
        bodies_.clear();
    }

    void addBody(std::shared_ptr<CelestialBody> body) {
        bodies_.push_back(body);
    }

    void step() {
        OctreeNode root(Vector3D(0, 0, 0), universeSize_);
        for (const auto& body : bodies_) {
            root.insert(body);
        }

        #pragma omp parallel for
        for (size_t i = 0; i < bodies_.size(); ++i) {
            if (bodies_[i]->getName() == "Sun") {
                bodies_[i]->acceleration_ = Vector3D(0, 0, 0);
                continue;
            }
            Vector3D force = root.calculateForce(*bodies_[i]);
            bodies_[i]->acceleration_ = force * (1.0 / bodies_[i]->getMass());
        }

        #pragma omp parallel for
        for (size_t i = 0; i < bodies_.size(); ++i) {
            if (bodies_[i]->getName() == "Sun") {
                bodies_[i]->position_ = Vector3D(0, 0, 0);
                bodies_[i]->velocity_ = Vector3D(0, 0, 0);
                continue;
            }
            bodies_[i]->updateState(Constants::TIME_STEP);
        }
    }

    json getSystemState() const {
        json state;
        for (const auto& body : bodies_) {
            state.push_back(body->toJson());
        }
        return state;
    }
};

class NewtonianSimulator {
private:
    std::vector<std::shared_ptr<CelestialBody>> bodies_;

    Vector3D calculateGravitationalForce(const CelestialBody& body1, const CelestialBody& body2) {
        Vector3D r = body2.getPosition() - body1.getPosition();
        double distance = r.magnitude();
        
        if (distance < (body1.getRadius() + body2.getRadius())) {
            return Vector3D(0, 0, 0);  
        }

        double forceMagnitude = Constants::G * body1.getMass() * body2.getMass() / 
                               (distance * distance);
        return r.normalize() * forceMagnitude;
    }

public:
    void addBody(std::shared_ptr<CelestialBody> body) {
        bodies_.push_back(body);
    }

    void step() {
        #pragma omp parallel for
        for (size_t i = 0; i < bodies_.size(); ++i) {
            if (bodies_[i]->getName() == "Sun") {
                bodies_[i]->acceleration_ = Vector3D(0, 0, 0);
                continue;
            }

            Vector3D totalForce(0, 0, 0);
            for (size_t j = 0; j < bodies_.size(); ++j) {
                if (i != j) {
                    Vector3D force = calculateGravitationalForce(*bodies_[i], *bodies_[j]);
                    totalForce = totalForce + force;
                    
                }
            }
            bodies_[i]->acceleration_ = totalForce * (1.0 / bodies_[i]->getMass());
        }

        #pragma omp parallel for
        for (size_t i = 0; i < bodies_.size(); ++i) {
            if (bodies_[i]->getName() == "Sun") {
                bodies_[i]->position_ = Vector3D(0, 0, 0);
                bodies_[i]->velocity_ = Vector3D(0, 0, 0);
                continue;
            }
            bodies_[i]->updateState(Constants::TIME_STEP);
        }
    }

    json getSystemState() const {
        json state;
        for (const auto& body : bodies_) {
            state.push_back(body->toJson());
        }
        return state;
    }

    void clear() {
        bodies_.clear();
    }
};

// 全局模拟器实例
NewtonianSimulator newtonianSimulator;
BarnesHutSimulator barnesHutSimulator;

int main() {
    httplib::Server svr;

    // 初始化���体数据
    auto initializeBodies = [](auto& simulator) {
        simulator.clear();  // 清除现有天体
        
        // 太阳
        auto sun = std::make_shared<CelestialBody>(
            "Sun", 1.989e30, 696340000,
            Vector3D(0, 0, 0), Vector3D(0, 0, 0)
        );

        // 水星 - 平均轨道速度: 47.87 km/s
        auto mercury = std::make_shared<CelestialBody>(
            "Mercury", 3.285e23, 2439700,
            Vector3D(57.9e9, 0, 0), Vector3D(0, 47.87e3, 0)
        );

        // 金星 - 平均轨道速度: 35.02 km/s
        auto venus = std::make_shared<CelestialBody>(
            "Venus", 4.867e24, 6051800,
            Vector3D(108.2e9, 0, 0), Vector3D(0, 35.02e3, 0)
        );

        // 地球 - 平均轨道速度: 29.78 km/s
        auto earth = std::make_shared<CelestialBody>(
            "Earth", 5.972e24, 6371000,
            Vector3D(149.6e9, 0, 0), Vector3D(0, 29.78e3, 0)
        );

        // 火星 - 平均轨道速度: 24.077 km/s
        auto mars = std::make_shared<CelestialBody>(
            "Mars", 6.39e23, 3389500,
            Vector3D(227.9e9, 0, 0), Vector3D(0, 24.077e3, 0)
        );

        // 木星 - 平均轨道速度: 13.07 km/s
        auto jupiter = std::make_shared<CelestialBody>(
            "Jupiter", 1.898e27, 69911000,
            Vector3D(778.5e9, 0, 0), Vector3D(0, 13.07e3, 0)
        );

        // 土星 - 平均轨道速度: 9.68 km/s
        auto saturn = std::make_shared<CelestialBody>(
            "Saturn", 5.683e26, 58232000,
            Vector3D(1.434e12, 0, 0), Vector3D(0, 9.68e3, 0)
        );

        // 天王星 - 平均轨道速度: 6.80 km/s
        auto uranus = std::make_shared<CelestialBody>(
            "Uranus", 8.681e25, 25362000,
            Vector3D(2.871e12, 0, 0), Vector3D(0, 6.80e3, 0)
        );

        // 海王星 - 平均轨道速度: 5.43 km/s
        auto neptune = std::make_shared<CelestialBody>(
            "Neptune", 1.024e26, 24622000,
            Vector3D(4.495e12, 0, 0), Vector3D(0, 5.43e3, 0)
        );

        simulator.addBody(sun);
        simulator.addBody(mercury);
        simulator.addBody(venus);
        simulator.addBody(earth);
        simulator.addBody(mars);
        simulator.addBody(jupiter);
        simulator.addBody(saturn);
        simulator.addBody(uranus);
        simulator.addBody(neptune);
    };

    // 初始化两种拟器
    initializeBodies(newtonianSimulator);
    initializeBodies(barnesHutSimulator);
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
/////////////////和前端进行通信//////////////////////////////
////////////////////////////////////////////////////////////


    svr.set_base_dir("./public");
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    svr.Get("/api/system-state", [](const httplib::Request& req, httplib::Response& res) {
        bool useBarnesHut = req.has_param("algorithm") && req.get_param_value("algorithm") == "barnes-hut";
        json state = useBarnesHut ? barnesHutSimulator.getSystemState() : newtonianSimulator.getSystemState();
        res.set_content(state.dump(), "application/json");
    });

    svr.Post("/api/simulate", [](const httplib::Request& req, httplib::Response& res) {
        bool useBarnesHut = req.has_param("algorithm") && req.get_param_value("algorithm") == "barnes-hut";
        if (useBarnesHut) {
            barnesHutSimulator.step();
            json state = barnesHutSimulator.getSystemState();
            res.set_content(state.dump(), "application/json");
        } else {
            newtonianSimulator.step();
            json state = newtonianSimulator.getSystemState();
            res.set_content(state.dump(), "application/json");
        }
    });

    svr.listen("localhost", 8081);
}
