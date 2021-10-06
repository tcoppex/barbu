#ifndef BARBU_ECS_COMPONENTS_SPHERE_COLLIDER_H_
#define BARBU_ECS_COMPONENTS_SPHERE_COLLIDER_H_

#include "ecs/component.h"
#include "glm/vec3.hpp"

// ----------------------------------------------------------------------------

//
//  Add a bounding sphere to test collision. 
//
class SphereColliderComponent final : public ComponentParams<Component::SphereCollider> {
 public:
  SphereColliderComponent() 
    : center_{0.0f, 0.0f, 0.0f}
    , radius_{1.0f}
  {}

  inline glm::vec3 const& center() const { return center_; }
  inline float radius() const { return radius_; }
  //inline glm::vec4 data() const { return glm::vec4( center_, radius_); }


  inline void set_center(glm::vec3 const& center) noexcept {
    center_ = center;
  }

  inline void set_radius(float radius) noexcept {
    radius_ = radius;
  }

  inline void set(glm::vec3 const& center, float radius) noexcept {
    set_center(center);
    set_radius(radius);    
  }

 private:
  glm::vec3 center_;
  float radius_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_COMPONENTS_SPHERE_COLLIDER_H_