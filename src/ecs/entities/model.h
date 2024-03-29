#ifndef BARBU_ECS_ENTITIES_MODEL_H_
#define BARBU_ECS_ENTITIES_MODEL_H_

#include "ecs/entity.h"
#include "ecs/components/visual.h"

// ----------------------------------------------------------------------------

//
//  A prebuilt entity that store a mesh.
//
class ModelEntity : public Entity {
 public:
  ModelEntity(std::string_view name, MeshHandle mesh)
    : Entity(name)
  {
    auto &visual = add<VisualComponent>();
    visual.setMesh(mesh);
  }

  MeshHandle mesh() {
    return get<VisualComponent>().mesh();
  }

  SkeletonHandle skeleton() {
    return mesh()->skeleton();
  }
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_ENTITIES_MODEL_H_