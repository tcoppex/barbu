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

  void init();

  /* Update the scene and its entity. */
  void update(float const dt, Camera const& camera);

  /* Add an entity to the scene hierarchy from its parent, or the root if none exists. */
  template<class T = Entity>
  std::enable_if_t<std::is_base_of_v<Entity, T>, std::shared_ptr<T>>
  addChildEntity(EntityHandle parent, std::shared_ptr<T> entity) noexcept {
    assert( nullptr != entity );
    assert( nullptr == entity->parent_ );

    if (nullptr == parent) {
      parent = root_;
    }

    // [fixme]
    // New entities don't have an index and corresponding perFrame internal structures
    // while the hierarchy has not been update. Hence using their perFrame data directly
    // afterwards, in the same frame, can crash the app.
    // One obvious solution would be to update the subhierarchy after every add.
    // + Or use two updates : one pre-user_update & one pre-use_draw calls.
    // + Or don't allow indexing use externally.
    entity->index_ = -1; //entities_.empty() ? 0 : static_cast<int32_t>(entities_.size()-1);

    entity->parent_ = parent;
    parent->children_.push_back( entity );
    entities_.push_back( entity );

    return entity;
  }

  /* Add an entity to the root. */
  template<class T = Entity>
  std::enable_if_t<std::is_base_of_v<Entity, T>, std::shared_ptr<T>>
  addEntity(std::shared_ptr<T> entity) noexcept {
    return addChildEntity( nullptr, entity);
  }

  /* Create an entity, empty or from a template, attached to its parent. */
  template<class T = Entity, class... U>
  std::enable_if_t<std::is_base_of_v<Entity, T>, std::shared_ptr<T>>
  createChildEntity(EntityHandle parent, U&&... u) {
    auto e = Entity::Create<T>( std::forward<U>(u)... ); 
    return e ? addChildEntity<T>(parent, e) : nullptr;
  }

  /* Create an entity, empty or from a template, attached to the root. */
  template<class T = Entity, class... U>
  std::enable_if_t<std::is_base_of_v<Entity, T>, std::shared_ptr<T>>
  createEntity(U&&... u) {
    auto e = Entity::Create<T>( std::forward<U>(u)... );
    return e ? addEntity<T>(e) : nullptr;
  }

  /* Remove an entity to the list of entity and update its relationship links. */
  void removeEntity(EntityHandle entity, bool bRecursively = false);

  /* Reset the entity local transform. */
  void resetEntity(EntityHandle entity, bool bRecursively = false);

  /* Create a model entity by importing an external model file. */
  EntityHandle importModel(std::string_view filename);

  /* Select all entities when true, deselect otherwise. */
  void toggleSelect(EntityHandle entity, bool status);

  /* Select all entities when true, deselect otherwise. */
  void toggleSelect(bool status);
  
  /* Select an entity. */
  inline void select(EntityHandle entity) { toggleSelect(entity, true); }

  /* Deselect an entity. */
  inline void deselect(EntityHandle entity) { toggleSelect(entity, false); }

  /* Select all entities. */
  inline void selectAll() { toggleSelect(true); }
  
  /* Deselect all entities. */
  inline void deselectAll() { toggleSelect(false); }

  /* Getters */

  /* Return the global matrix for the given entity index. */
  inline glm::mat4& globalMatrix(int32_t index) { return frame_.globals[index]; }
  inline glm::mat4 const& globalMatrix(int32_t index) const { return frame_.globals[index]; }

  /* Return the entity position in world space. */
  inline glm::vec3 globalPosition(EntityHandle e) const {
    return glm::vec3(parentGlobalMatrix(e) * glm::vec4(e->position(), 1.0)); //
  }

  /* Return the entity centroid in world space. */
  inline glm::vec3 globalCentroid(EntityHandle e) const {
    return glm::vec3(parentGlobalMatrix(e) * glm::vec4(e->centroid(), 1.0)); //
  }

  /* Return the first entity of the list, if any. */
  inline EntityHandle first() const { return entities_.empty() ? nullptr : entities_.front(); } //
  
  /* Return the list of all entities. */
  inline EntityList_t const& all() const { return entities_; }

  /* Return the list of selected entities. */
  inline EntityList_t const& selected() const { return frame_.selected; }
  
  /* Return the list of drawable entities. */
  inline EntityList_t const& drawables() const { return frame_.drawables; }
  
  /* Return the list of collidable entities. */
  inline EntityList_t const& colliders() const { return frame_.colliders; }

  /* Return true when the entity is selected. */
  bool isSelected(EntityHandle entity) const;

  // Return the pivot / centroid of the scene from the entities root.
  // If selected is true, will be limited to the selection.
  glm::vec3 pivot(bool selected=true) const;
  glm::vec3 centroid(bool selected=true) const;


  /* Experimental */

  /* Return the next entity in hierarchy given a base and a step size. */
  EntityHandle next(EntityHandle entity, int32_t step = 1) const;

  /* Add a bounding model entity for physic collision. */
  EntityHandle add_bounding_sphere(float radius = 1.0f); //

  /* Render debug rigs from the scene for debug display. */
  void renderDebugRigs() const; //

  /* Render debug colliders from the scene for debug display. */
  void renderDebugColliders() const; //

  /* Update / display selected gizmos. */
  void processGizmos(bool use_centroid = false);

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

  /* Hierarchically prefix-update entities. */
  void updateHierarchy(float const dt);
  void subUpdateHierarchy(float const dt, EntityHandle entity, int &index);

  /* Update locals matrices based on their modified globals. */
  void updateSelectedLocalMatrices();

  /* Sort drawable entities front to back, relative to the camera. */
  void sortDrawables(Camera const& camera);

  /* Return the entity's parent global matrix, or the identity if none exists. */
  inline glm::mat4 const& parentGlobalMatrix(EntityHandle e) const { 
    if (auto index = e->parent()->index(); index >= 0) {
      return globalMatrix(index);
    }
    return sIdentity; 
  }

  /* Render a node depending on its relations in hierarchy. */
  void renderDebugNode(EntityHandle node) const;
  
  EntityHandle root_;                 //< Entry to the entity hierarchy.
  EntityList_t entities_;             //< List of all current entities.
  PerFrame_t frame_;                  //< Holds per frame data.
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_SCENE_HIERARCHY_H_
