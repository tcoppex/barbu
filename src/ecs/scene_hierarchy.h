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
  // Default container for entities.
  using EntityList_t = std::list<EntityHandle>;

  // Identity matrix.
  static constexpr glm::mat4 sIdentity{1.0f};

  // User interface data.
  std::shared_ptr<views::SceneHierarchyView> ui_view = nullptr;
  friend class views::SceneHierarchyView;

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
  // specified. 
  void addEntity(EntityHandle entity, EntityHandle parent = nullptr);

  // Remove an entity to the list of entity and update its relationship links.
  void removeEntity(EntityHandle entity, bool bRecursively = false);

  // Reset the entity local transform.
  void resetEntity(EntityHandle entity, bool bRecursively = false);

  // Create a model entity by importing an external model file.
  EntityHandle importModel(std::string_view filename);
  
  // Select / Unselect entities depending on status.
  void select(EntityHandle entity, bool status);
  void selectAll(bool status);

  // ----------------------

  /* Getters */

  // Return the global matrix for the given entity index.
  inline glm::mat4& globalMatrix(int32_t index) { return frame_.globals[index]; }
  inline glm::mat4 const& globalMatrix(int32_t index) const { return frame_.globals[index]; }

  // Return the entity position in world space.
  inline glm::vec3 globalPosition(EntityHandle e) const {
    return glm::vec3(parentGlobalMatrix(e) * glm::vec4(e->position(), 1.0)); //
  }

  // Return the entity centroid in world space.
  inline glm::vec3 globalCentroid(EntityHandle e) const {
    return glm::vec3(parentGlobalMatrix(e) * glm::vec4(e->centroid(), 1.0)); //
  }

  // Return the first entity of the list, if any.
  EntityHandle first() const { return entities_.empty() ? nullptr : entities_.front(); } //
  
  // Return the list of entities.
  inline EntityList_t const& all() const { return entities_; }

  inline EntityList_t const& selected() const { return frame_.selected; }
  inline EntityList_t const& drawables() const { return frame_.drawables; }
  inline EntityList_t const& colliders() const { return frame_.colliders; }

  // Return true when the entity is selected.
  bool isSelected(EntityHandle entity) const;

  // Return the pivot / centroid of the scene from the entities root.
  // If selected is true, will be limited to the selection.
  glm::vec3 pivot(bool selected=true) const;
  glm::vec3 centroid(bool selected=true) const;

  // ----------------------

  /* Experimental */

  EntityHandle next(EntityHandle entity, int32_t step = 1) const;

  // Add a bounding model entity for physic collision.
  EntityHandle add_bounding_sphere(float radius = 1.0f); //

  // Render availables rigs from the scene for debug display.
  void renderDebugRigs() const; //
  void renderDebugColliders() const; //

  // Update / display selected gizmos.
  void gizmos(bool use_centroid);

  // ----------------------

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

    // Entities with colliders.
    EntityList_t colliders;

    
    void clear() {
      assert(matrices_stack.empty());
      globals.clear();
      selected.clear();
      drawables.clear();
      colliders.clear();
    }
  };

  // Create a model entity from a mesh and a basename.
  EntityHandle createModelEntity(std::string const& basename, MeshHandle mesh) noexcept;

  EntityHandle createLightEntity(std::string const& basename) noexcept; // [wip]

  // Hierarchically prefix-update entities.
  void updateHierarchy(float const dt);
  void subUpdateHierarchy(float const dt, EntityHandle entity, int &index);

  // Update locals matrices based on their modified globals.
  void updateSelectedLocalMatrices();

  // Sort drawable entities front to back, relative to the camera.
  void sortDrawables(Camera const& camera);

  // Return the entity's parent global matrix, or the identity if none exists.
  inline glm::mat4 const& parentGlobalMatrix(EntityHandle e) const { 
    if (auto index = e->parent()->index(); index >= 0) {
      return globalMatrix(index);
    }
    return sIdentity; 
  }

  // Render a node depending on its relations in hierarchy.
  void render_debug_node(EntityHandle node) const;

  // Entry to the entity hierarchy.
  EntityHandle root_;

  // List of all current entities.
  EntityList_t entities_;

  // Holds per frame data.
  PerFrame_t frame_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_SCENE_HIERARCHY_H_
