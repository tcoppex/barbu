#ifndef BARBU_CORE_CAMERA_H_
#define BARBU_CORE_CAMERA_H_

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#ifdef WIN32
  // MSVC like to define those for some reasons.
  // (could be bypassed if windows.h is included afterward)
  #ifdef far
    #undef far
  #endif
  #ifdef near
    #undef near
  #endif
#endif

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
    virtual void get_view_matrix(float *m) = 0; 
    virtual glm::vec3 target() const = 0; //
  };

 public:
  Camera()
    : controller_(nullptr)
    , fov_(0.0f)
    , width_(0)
    , height_(0)
    , linear_params_{0.0f} 
  {}

  Camera(ViewController *controller) 
    : Camera()
  {
    controller_ = controller;
  }

  inline bool initialized() const {
    return (fov_ > 0.0f) && (width_ > 0) && (height_ > 0);
  }

  void set_perspective(float fov, int32_t w, int32_t h, float near, float far) {
    assert( fov > 0.0f );
    assert( (w > 0) && (h > 0) );
    assert( (far - near) > 0.0f );

    fov_    = fov;
    width_  = w;
    height_ = h;
    // Projection matrix.
    float const ratio = static_cast<float>(width_) / static_cast<float>(height_);
    proj_ = glm::perspective( fov_, ratio, near, far);
    bUseOrtho_ = false;
    // Linearization parameters.
    float const A  = far / (far - near);
    linear_params_ = glm::vec4( near, far, A, - near * A);
  }

  void set_default() {
    set_perspective( kDefaultFOV, kDefaultSize, kDefaultSize, kDefaultNear, kDefaultFar);
  }

  // Update controller and rebuild all matrices.
  void update(float dt) {
    if (controller_) {
      controller_->update(dt);
    }
    rebuild();
  }

  // Rebuild all matrices.
  void rebuild(bool bRetrieveView=true) {
    if (controller_ && bRetrieveView) {
      controller_->get_view_matrix(glm::value_ptr(view_));
    }
    world_    = glm::inverse(view_); // costly
    viewproj_ = proj_ * view_;
  }

  inline ViewController *controller() { return controller_; }
  inline ViewController const* controller() const { return controller_; }

  inline void set_controller(ViewController *controller) { controller_ = controller; }

  inline float fov() const noexcept { return fov_; }

  inline int32_t width() const noexcept { return width_; }
  inline int32_t height() const noexcept { return height_; }
  inline float aspect() const { return static_cast<float>(width_) / static_cast<float>(height_); }

  inline float near() const noexcept { return linear_params_.x; }
  inline float far() const noexcept { return linear_params_.y; }

  inline glm::vec4 const& linearization_params() const {
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

  inline bool is_ortho() const noexcept { return bUseOrtho_; }

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

// ----------------------------------------------------------------------------

#endif // BARBU_CORE_CAMERA_H_
