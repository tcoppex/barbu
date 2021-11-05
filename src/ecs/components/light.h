#ifndef BARBU_ECS_COMPONENTS_LIGHT_H_
#define BARBU_ECS_COMPONENTS_LIGHT_H_

#include "ecs/component.h"
#include "shaders/shared/lighting/interop.h"

// ----------------------------------------------------------------------------

enum class LightType : uint8_t {
  Directional     = LIGHT_TYPE_DIRECTIONAL,
  Point           = LIGHT_TYPE_POINT,

  kCount
};

// ----------------------------------------------------------------------------

class LightComponent final : public ComponentParams<Component::Light> {
 static constexpr LightType kDefaultType      {LightType::Point}; 
 static constexpr glm::vec3 kDefaultColor     {1.0f, 0.984f, 0.941f}; // 0xfffbf0
 static constexpr float     kDefaultIntensity {1.0f};

 public:
  LightComponent()
    : type_{kDefaultType}
    , color_{kDefaultColor}
    , intensity_{kDefaultIntensity}
  {}

  inline LightType type() const noexcept { return type_; }
  inline glm::vec3 const& color() const noexcept { return color_; }
  inline float intensity() const noexcept { return intensity_; }

  void setType(LightType type) noexcept {
    type_ = type;
  }

  void setColor(glm::vec3 const& color) noexcept {
    color_ = color;
  }

  void setIntensity(float intensity) noexcept {
    intensity_ = intensity;
  }

 private:
  LightType   type_;
  glm::vec3   color_;
  float       intensity_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_COMPONENTS_LIGHT_H_