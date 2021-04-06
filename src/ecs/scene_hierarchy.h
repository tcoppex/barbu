#ifndef BARBU_ECS_SCENE_HIERARCHY_H_
#define BARBU_ECS_SCENE_HIERARCHY_H_

#include "ecs/ecs.h"

#include <cassert>
#include <vector>
#include <list>
#include <stack>

class Camera;
class UIView;
namespace views {
class SceneHierarchyView;
}

// ----------------------------------------------------------------------------

class SceneHierarchy {
 public:
  using EntityList_t = std::list<EntityHandle>;

 public:
  SceneHierarchy() {
    root_ = std::make_shared<Entity>();
    root_->parent_ = nullptr;
  }

  ~SceneHierarchy();

  void init(); //

  // Update the scene and its entity.
  void update(float const dt, Camera const& camera);

  // Add an entity to the scene hierarchy placed on the root when no parent is
  // specififed. 
  void add_entity(EntityHandle entity, EntityHandle parent = nullptr);

  // Remove an entity to the list of entity and update its relationship links.
  void remove_entity(EntityHandle entity, bool bRecursively = false);

  // Create a model entity by importing an external model file.
  bool import_model(std::string_view filename);

  // Update locals matrices based on their modified globals.
  void update_selected_local_matrices();
  
  glm::mat4 & global_matrix(int32_t index) { return frame_.globals[index]; } //
  glm::mat4 const& global_matrix(int32_t index) const { return frame_.globals[index]; } //

  EntityList_t const& selected() const { return frame_.selected; }
  EntityList_t const& drawables() const { return frame_.drawables; }
  
  // [wip] Add a bounding model entity for physic collision.
  EntityHandle add_bounding_sphere();

 private:
  // Set of buffers modified each frame.
  struct PerFrame_t {
    // Entities' current frame global matrices.
    std::vector<glm::mat4> globals; //

    // Matrices stack to compute transforms.
    // [ using the global matrices and parent frame index we could bypass it ]
    std::stack<glm::mat4> matrices_stack;

    // Currently selected entities.
    EntityList_t selected;

    // Camera-dependent entities to render.
    EntityList_t drawables;
    
    void clear() {
      assert(matrices_stack.empty());
      globals.clear();
      selected.clear();
      drawables.clear();
    }
  };

  // Create a model entity from a mesh and a basename.
  EntityHandle create_model_entity(std::string const& basename, MeshHandle mesh);

  // Hierarchically prefix-update entities.
  void update_hierarchy(float const dt);
  void update_sub_hierarchy(float const dt, EntityHandle entity, int &index);

  // Sort drawable entities front to back, relative to the camera.
  void sort_drawables(Camera const& camera);

  // Entry to the entity hierarchy.
  EntityHandle root_;

  // List of all current entities.
  EntityList_t entities_;

  // Holds per frame data.
  PerFrame_t frame_;

 public:
  UIView *ui_view = nullptr;
  friend class views::SceneHierarchyView;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_SCENE_HIERARCHY_H_
