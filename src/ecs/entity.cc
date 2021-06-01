#include "ecs/entity.h"

#include "ecs/components/visual.h"
#include "ecs/components/skin.h"
#include "ecs/components/sphere_collider.h"

// ----------------------------------------------------------------------------

glm::vec3 Entity::centroid() const { 
  if (has<VisualComponent>()) {
    return get<VisualComponent>().mesh()->centroid();
  } else if (has<SphereColliderComponent>()) {
    return get<SphereColliderComponent>().center();
  }
  return glm::vec3(0.0f); 
}

// ----------------------------------------------------------------------------
