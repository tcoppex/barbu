#ifndef BARBU_CORE_CAMERA_H_
#define BARBU_CORE_CAMERA_H_

#include <memory>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// #ifdef WIN32
//   // MSVC likes to define those for some reasons.
//   // (could be bypassed if windows.h is included afterwards)
//   #ifdef far
//     #undef far
//   #endif
//   #ifdef near
//     #undef near
//   #endif
// #endif

// ----------------------------------------------------------------------------

class Camera {
 public:
  static constexpr float kDefaultFOV    = glm::radians(90.0f);
  static constexpr int32_t kDefaultSize = 512;
  static constexpr float kDefaultNear   = 0.1f;
  static constexpr float kDefaultFar    = 500.0f;

  class ViewController {
   public:
    virtual ~ViewController() {}
    virtual void update(float dt) {} 
    virtual void getViewMatrix(float *m) = 0; 
    virtual glm::vec3 target() const = 0; //
  };

 public:
  Camera()
    : controller_{nullptr}
    , fov_(0.0f)
    , width_(0)
    , height_(0)
    , linear_params_{0.0f, 0.0f, 0.0f, 0.0f} 
  {}

  Camera(ViewController *controller) 
    : Camera()
  {
    controller_ = controller;
  }

  inline bool initialized() const noexcept {
    return (fov_ > 0.0f) && (width_ > 0) && (height_ > 0);
  }

  void setPerspective(float fov, int32_t w, int32_t h, float znear, float zfar) {
    assert( fov > 0.0f );
    assert( (w > 0) && (h > 0) );
    assert( (zfar - znear) > 0.0f );

    fov_    = fov;
    width_  = w;
    height_ = h;
    // Projection matrix.
    float const ratio = static_cast<float>(width_) / static_cast<float>(height_);
    proj_ = glm::perspective( fov_, ratio, znear, zfar);
    bUseOrtho_ = false;
    // Linearization parameters.
    float const A  = zfar / (zfar - znear);
    linear_params_ = glm::vec4( znear, zfar, A, - znear * A);
  }

  void setPerspective(float fov, glm::ivec2 const& res, float znear, float zfar) {
    setPerspective( fov, res.x, res.y, znear, zfar);
  }

  void setDefault() {
    setPerspective( kDefaultFOV, kDefaultSize, kDefaultSize, kDefaultNear, kDefaultFar);
  }

  // Update controller and rebuild all matrices.
  void update(float dt) {
    if (controller_) {
      controller_->update(dt);
    }
    rebuild();
  }

  // Rebuild all matrices.
  void rebuild(bool bRetrieveView = true) {
    if (controller_ && bRetrieveView) {
      controller_->getViewMatrix(glm::value_ptr(view_));
    }
    world_    = glm::inverse(view_); // costly
    viewproj_ = proj_ * view_;
  }

  inline ViewController *controller() { return controller_; }
  inline ViewController const* controller() const { return controller_; }

  inline void setController(ViewController *controller) { controller_ = controller; }

  inline float fov() const noexcept { return fov_; }

  inline int32_t width() const noexcept { return width_; }
  inline int32_t height() const noexcept { return height_; }
  inline float aspect() const { return static_cast<float>(width_) / static_cast<float>(height_); }

  inline float znear() const noexcept { return linear_params_.x; }
  inline float zfar() const noexcept { return linear_params_.y; }

  inline glm::vec4 const& linearizationParams() const {
    return linear_params_;
  }

  inline glm::mat4 const& view() const noexcept { return view_; }
  inline glm::mat4 const& world() const noexcept { return world_; }
  inline glm::mat4 const& proj() const noexcept { return proj_; }
  inline glm::mat4 const& viewproj() const noexcept { return viewproj_; }

  inline glm::vec3 position() const noexcept  { return glm::vec3(world_[3]); } //
  inline glm::vec3 direction() const noexcept { return glm::normalize(-glm::vec3(world_[2])); }

  inline glm::vec3 target() const noexcept {
    return controller_ ? controller_->target() : position() + 3.0f*direction(); //
  }

  inline bool isOrtho() const noexcept { return bUseOrtho_; }

 private:
  ViewController *controller_;

  float fov_;
  int32_t width_; //
  int32_t height_; //
  glm::vec4 linear_params_;

  glm::mat4 view_;
  glm::mat4 world_;
  glm::mat4 proj_;
  glm::mat4 viewproj_;

  bool bUseOrtho_;
};

using CameraHandle = std::shared_ptr<Camera>;

// ----------------------------------------------------------------------------

#endif // BARBU_CORE_CAMERA_H_
