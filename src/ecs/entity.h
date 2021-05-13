#ifndef BARBU_ECS_ENTITY_H_
#define BARBU_ECS_ENTITY_H_

#include <cassert>
#include <array>
#include <vector>
#include <unordered_map>
#include <string_view>

#include "ecs/component.h"
#include "ecs/components/transform.h"
#include "ecs/components/visual.h" //

// ----------------------------------------------------------------------------

class Entity;
using EntityHandle = std::shared_ptr<Entity>;

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

 public:
  Entity() = default;

  Entity(std::string_view name)
    : name_(name)
  {
    add<TransformComponent>();
  }

  virtual ~Entity() {}

  virtual void update(float const dt) {}

  inline EntityHandle parent() const { return parent_; }
  
  inline EntityChildren_t const& children() const { return children_; }

  inline std::string const& name() const { return name_; }

  inline int32_t index() const { return index_; }

  // -- Components access.

  // Return true if the entity possess the component.
  template<typename T> bool has() const noexcept {
    return static_cast<bool>(components_[T::Type]);
  }
  
  // Return a reference to the component [ return shared_ptr<T> instead ? ].
  template<typename T> T & get() {
    assert( has<T>() );
    return static_cast<T&>(*components_[T::Type]);
  }

  // Return a constant reference to the component.
  template<typename T> T const& get() const {
    assert( has<T>() );
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
    static_assert(T::Type != Component::Type::Transform);
    components_[T::Type].reset();
  }

  // -- Transform component quick access points.

  inline TransformComponent & transform() { return get<TransformComponent>(); }
  inline TransformComponent const& transform() const { return get<TransformComponent>(); }

  inline glm::mat4 & local_matrix() { return transform().matrix(); }
  inline glm::mat4 const& local_matrix() const { return transform().matrix(); }
  
  inline glm::vec3 position() const { return transform().position(); }
  inline void set_position(glm::vec3 const& pos) { transform().set_position(pos); }

  // Return the barycenter of an object, depending on its components. 
  inline glm::vec3 centroid() const { 
    if (has<VisualComponent>()) {
      return get<VisualComponent>().mesh()->centroid();
    }
    return position(); 
  }

  // -- Miscs.

  // Let view the entity as a subtype. [bug prone]
  template<typename T> T& as() {
    return *((T*)this);
  }

 protected:
  EntityHandle parent_ = nullptr;
  EntityChildren_t children_;

  std::string name_;
  int32_t index_ = -1;        // index in the scene hierarchy per-frame structure.

 private:
  // [ should probably use a map, and be set externally ]
  using ComponentBuffer = std::array< ComponentHandle, Component::kNumComponentType >;

  ComponentBuffer components_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_ENTITY_H_