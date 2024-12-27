#pragma once

#include "Vector3D.hpp"
#include "CelestialBody.hpp"
#include "Config.hpp"
#include <array>
#include <memory>
#include <list>

namespace GEngine {

class OctreeNode {
private:
    Vector3D center_;
    double size_;
    double totalMass_;
    Vector3D centerOfMass_;
    std::array<std::unique_ptr<OctreeNode>, 8> children_;
    std::list<std::shared_ptr<CelestialBody>> bodies_;

    int getOctant(const Vector3D& position) const;
    void subdivide();

public:
    OctreeNode(const Vector3D& center, double size)
        : center_(center), size_(size), totalMass_(0) {}

    void insert(std::shared_ptr<CelestialBody> body);
    Vector3D calculateForce(const CelestialBody& body) const;

    double getTotalMass() const { return totalMass_; }
    const Vector3D& getCenterOfMass() const { return centerOfMass_; }
    double getSize() const { return size_; }
    const std::unique_ptr<OctreeNode>& getChild(int index) const { 
        return children_[index]; 
    }
};

} 