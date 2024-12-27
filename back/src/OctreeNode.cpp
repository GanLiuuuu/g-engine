#include "../include/OctreeNode.hpp"
#include "../include/Config.hpp"

namespace GEngine {

int OctreeNode::getOctant(const Vector3D& position) const {
    int octant = 0;
    if (position.x() > center_.x()) octant |= 1;
    if (position.y() > center_.y()) octant |= 2;
    if (position.z() > center_.z()) octant |= 4;
    return octant;
}

void OctreeNode::subdivide() {
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

void OctreeNode::insert(std::shared_ptr<CelestialBody> body) {
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

Vector3D OctreeNode::calculateForce(const CelestialBody& body) const {
    if (bodies_.empty() && !children_[0]) {
        return Vector3D(0, 0, 0);
    }

    Vector3D r = centerOfMass_ - body.getPosition();
    double distance = r.magnitude();
    
    if (size_ / distance < SimulationConfig::getInstance().barnesHutTheta) {
        if (distance > 0) { 
            double forceMagnitude = SimulationConfig::getInstance().gravityConstant * 
                                  totalMass_ * body.getMass() / 
                                  (distance * distance);
            return r.normalize() * forceMagnitude;
        }
        return Vector3D(0, 0, 0);
    }

    Vector3D totalForce(0, 0, 0);
    if (children_[0]) {
        for (const auto& child : children_) {
            totalForce = totalForce + child->calculateForce(body);
        }
    } else {
        for (const auto& otherBody : bodies_) {
            if (otherBody->getName() != body.getName()) { 
                Vector3D r = otherBody->getPosition() - body.getPosition();
                double distance = r.magnitude();
                if (distance > 0) {
                    double forceMagnitude = SimulationConfig::getInstance().gravityConstant * 
                                          otherBody->getMass() * body.getMass() / 
                                          (distance * distance);
                    totalForce = totalForce + r.normalize() * forceMagnitude;
                }
            }
        }
    }
    return totalForce;
}

} 