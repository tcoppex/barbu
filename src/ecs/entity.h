#ifndef BARBU_ECS_ENTITY_H_
#define BARBU_ECS_ENTITY_H_

#include <cassert>
#include <array>
#include <vector>
#include <string>
#include <string_view>

#include "ecs/component.h"
#include "ecs/components/transform.h"

#include "ecs/entity-fwd.h"
class SceneHierarchy;

// ----------------------------------------------------------------------------

//
//  An entity define the representation of an object in 3D space that
//  communicate using a set of components.   
//  Each entities possess at least the transform component.
//
//  Note : Ideally an entity should be empty and its components added in
//         a larger buffer in the SceneHierarchy.
//
class Entity {
 public:
  friend class SceneHierarchy;

  using EntityChildren_t = std::vector<EntityHandle>;

  /* Wrapper around specific EntityHandle allocator. */
  template<class T = Entity, class... U> 
  static 
  std::enable_if_t<std::is_base_of_v<Entity, T>, std::shared_ptr<T>> 
  Create(U&&... u) {
    return std::make_shared<T>(std::forward<U>(u)...);
  }

 public:
  Entity() = default;

  Entity(std::string_view name)
    : name_(name)
  {
    add<TransformComponent>();
  }

  virtual ~Entity() {}

  virtual void update(float const dt) {}

  // -- Getters.

  inline EntityHandle parent() const { return parent_; }
  
  inline int32_t nchildren() const { return static_cast<int32_t>(children_.size()); }

  inline EntityChildren_t const& children() const { return children_; }

  inline EntityHandle child(int32_t index) { return (index < nchildren()) ? children_[index] : nullptr; }

  inline std::string const& name() const { return name_; }

  inline int32_t index() const { return index_; }

  inline bool indexed() const { return index_ > -1; }

  // -- Components access.

  /* Return true if the entity possess the component. */
  template<typename T> 
  std::enable_if_t<std::is_base_of_v<Component, T>, bool> has() const noexcept {
    return static_cast<bool>(components_[T::Type]);
  }
  
  /* Return a reference to the component [ return shared_ptr<T> instead ? ]. */
  template<typename T> 
  std::enable_if_t<std::is_base_of_v<Component, T>, T&> get() {
    assert( has<T>() );
    return static_cast<T&>(*components_[T::Type]);
  }

  /* Return a constant reference to the component. */
  template<typename T> 
  std::enable_if_t<std::is_base_of_v<Component, T>, T const&> get() const {
    assert( has<T>() );
    return static_cast<T&>(*components_[T::Type]);
  }

  /* Add then return the given component to the entity. */
  template<typename T> 
  std::enable_if_t<std::is_base_of_v<Component, T>, T&> add() {
    if (!has<T>()) {
      components_[T::Type] = std::make_unique<T>();
    }
    return get<T>();
  }

  /* Remove the given component. */
  template<typename T>
  std::enable_if_t<std::is_base_of_v<Component, T>> remove() {
    static_assert(T::Type != Component::Type::Transform);
    components_[T::Type].reset();
  }

  // -- Transform component quick access points.

  inline TransformComponent & transform() { return get<TransformComponent>(); }
  inline TransformComponent const& transform() const { return get<TransformComponent>(); }

  inline glm::mat4 & localMatrix() { return transform().matrix(); }
  inline glm::mat4 const& localMatrix() const { return transform().matrix(); }
  
  inline glm::vec3 position() const { return transform().position(); }
  inline void setPosition(glm::vec3 const& pos) { transform().setPosition(pos); }

  /* Return the barycenter of an object in local space, depending on its components. */
  glm::vec3 centroid() const;

  // -- Miscs.

  /* Let view the entity as a subtype. [bug prone] */
  template<typename T> T& as() {
    return *((T*)this);
  }

 protected:
  EntityHandle parent_ = nullptr;
  EntityChildren_t children_;
  
  std::string name_;
  int32_t index_ = -1;                //< index in the scene hierarchy per-frame structure.

 private:
  // [ should probably use a map, and be set externally ]
  using ComponentBuffer = std::array< ComponentHandle, Component::kCount >;

  ComponentBuffer components_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_ENTITY_H_