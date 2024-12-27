#pragma once

#include <cmath>
#include <nlohmann/json.hpp>

namespace GEngine {

class Vector3D {
private:
    double x_, y_, z_;

public:
    Vector3D(double x = 0, double y = 0, double z = 0) : x_(x), y_(y), z_(z) {}

    // 基本访问器
    double x() const { return x_; }
    double y() const { return y_; }
    double z() const { return z_; }

    // 运算符重载
    Vector3D operator+(const Vector3D& other) const {
        return Vector3D(x_ + other.x_, y_ + other.y_, z_ + other.z_);
    }

    Vector3D operator-(const Vector3D& other) const {
        return Vector3D(x_ - other.x_, y_ - other.y_, z_ - other.z_);
    }

    Vector3D operator*(double scalar) const {
        return Vector3D(x_ * scalar, y_ * scalar, z_ * scalar);
    }

    // 向量操作
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

    nlohmann::json toJson() const {
        return {x_, y_, z_};
    }

    static Vector3D fromJson(const nlohmann::json& j) {
        return Vector3D(j[0], j[1], j[2]);
    }
};

} 