#ifndef BARBU_ECS_COMPONENT_H_
#define BARBU_ECS_COMPONENT_H_

#include <cassert>
#include <memory>

// ----------------------------------------------------------------------------

class Component {
 public:
  enum Type {
    Transform,
    Visual,
    Skin,
    SphereCollider,

    kCount,
    kUndefined
  };

 public:
  virtual ~Component() = default;

  virtual Type type() const noexcept = 0;
};

using ComponentHandle = std::unique_ptr<Component>;

// ----------------------------------------------------------------------------

//
//  Parametrized Component template to define internal type 
//  (both at compile time & via polymorphism).
//
template<Component::Type tType>
class ComponentParams : public Component {
 public:
  static constexpr Component::Type Type = tType;

  Component::Type type() const noexcept final {
    return tType;
  }
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_COMPONENT_H_