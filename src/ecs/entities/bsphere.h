#ifndef BARBU_ECS_ENTITIES_BSPHERE_H_
#define BARBU_ECS_ENTITIES_BSPHERE_H_

#include "ecs/entity.h"
#include "ecs/components/sphere_collider.h"

// ----------------------------------------------------------------------------

//
//  A prebuilt entity that store a bounding sphere collider.
//
class BSphereEntity : public Entity {
 private:
  // Default name given to bounding sphere entities.
  static constexpr char const* kDefaultBoundingSphereEntityName{ "BSphere" };

 public:
  BSphereEntity(std::string_view _name, float _radius)
    : Entity(_name)
  {
    auto &sphere = add<SphereColliderComponent>();
    sphere.setRadius(_radius);
  }

  BSphereEntity(float _radius)
    : BSphereEntity(kDefaultBoundingSphereEntityName, _radius)
  {}

  float radius() const {
    return get<SphereColliderComponent>().radius();
  }
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_ENTITIES_BSPHERE_H_