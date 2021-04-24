#ifndef BARBU_ECS_ENTITY_H_
#define BARBU_ECS_ENTITY_H_

#include <cassert>
#include <array>
#include <vector>
#include <string_view>

#include "ecs/component.h"
#include "ecs/components/transform.h"

// ----------------------------------------------------------------------------

class Entity;
using EntityHandle = std::shared_ptr<Entity>;

class SceneHierarchy;

// ----------------------------------------------------------------------------

//
//  An entity define the representation of an object in 3D space that
//  communicate using a set of components. 
//  
//  Each entities possessed at least the transform component.
//
class Entity {
 public:
  friend class SceneHierarchy;

  using EntityChildren_t = std::vector<EntityHandle>;

 public:
  Entity() = default; //

  Entity(std::string_view name)
    : name_(name)
  {
    add<TransformComponent>();
  }

  virtual ~Entity() {}

  virtual void update(float const dt) {}

  EntityHandle parent() const { return parent_; }
  
  EntityChildren_t const& children() const { return children_; }

  std::string const& name() const { return name_; }

  int32_t index() const { return index_; }

  // Return true if the entity possess the component.
  template<typename T> bool has() const noexcept {
    return static_cast<bool>(components_[T::Type]);
  }
  
  // Return a reference to the component [ return shared_ptr<T> instead ? ].
  template<typename T> T & get() {
    return static_cast<T&>(*components_[T::Type]);
  }

  // Return a constant reference to the component.
  template<typename T> T const& get() const {
    return static_cast<T&>(*components_[T::Type]);
  }

  // Add then return the given component to the entity.
  template<typename T> T & add() {
    if (!has<T>()) {
      components_[T::Type] = std::make_unique<T>();
    }
    return get<T>();
  }

  // Remove the given component.
  template<typename T> void remove() {
    components_[T::Type].reset();
  }

  inline TransformComponent & transform() { return get<TransformComponent>(); }
  inline TransformComponent const& transform() const { return get<TransformComponent>(); }

  inline glm::mat4 & local_matrix() { return transform().matrix(); }
  inline glm::mat4 const& local_matrix() const { return transform().matrix(); }
  
  inline glm::vec3 pivot() const { return transform().position(); }
  virtual glm::vec3 centroid() const { return pivot(); }

 protected:
  EntityHandle parent_ = nullptr;
  EntityChildren_t children_;

  std::string name_;
  int32_t index_ = -1;        // index in the scene hierarchy per-frame structure.

 private:
  using ComponentBuffer = std::array< ComponentHandle, Component::kNumComponentType >;
  
  ComponentBuffer components_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_ENTITY_H_