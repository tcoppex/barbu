#ifndef BARBU_ECS_COMPONENTS_TRANSFORM_H_
#define BARBU_ECS_COMPONENTS_TRANSFORM_H_

#include "ecs/component.h"

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

// ----------------------------------------------------------------------------

//
//  Base component use by every entity. Describes a 3d coordinates space 
//  as a matrix 4x4.
//
class TransformComponent final : public ComponentParams<Component::Transform> {
 public:
  TransformComponent() 
    : matrix_{1.0f}
  {}

  inline glm::mat4& matrix() { return matrix_; }
  inline glm::mat4 const& matrix() const { return matrix_; }

  inline glm::vec3 position() const { return matrix_[3]; }
  inline glm::vec3 front() const { return -matrix_[2]; }

 private:
  glm::mat4 matrix_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_COMPONENTS_TRANSFORM_H_