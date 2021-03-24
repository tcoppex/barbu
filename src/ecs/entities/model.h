#ifndef BARBU_ECS_ENTITIES_MODEL_H_
#define BARBU_ECS_ENTITIES_MODEL_H_

#include "ecs/entity.h"
#include "ecs/components/visual.h"

// ----------------------------------------------------------------------------

// [ Work in progress ]

class ModelEntity : public Entity {
 public:
  ModelEntity(std::string_view name, MeshHandle mesh)
    : Entity(name)
  {
    auto &visual = add<VisualComponent>();
    visual.set_mesh(mesh);
  }

  MeshHandle mesh() {
    return get<VisualComponent>().mesh();
  }
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_ENTITIES_MODEL_H_