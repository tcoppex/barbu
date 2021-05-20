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
  static constexpr int kDefaultSize = 256;

  class ViewController {
   public:
    virtual ~ViewController() {}
    virtual void update(float dt) = 0; 
    virtual void get_view_matrix(float *m) = 0; 
    virtual glm::vec3 target() const = 0;
  };

 public:
  Camera()
    : controller_(nullptr)
    , width_(kDefaultSize)
    , height_(kDefaultSize)
    , fov_(1.0f)
    , linear_params_{0.0f} 
  {}

  Camera(ViewController *controller) 
    : Camera()
  {
    controller_ = controller;
  }

  void set_perspective(float fov, int w, int h, float near, float far) {
    fov_    = fov;
    width_  = w;
    height_ = h;
    float const ratio = static_cast<float>(width_) / static_cast<float>(height_);
    float const A     = far / (far - near);
    linear_params_ = glm::vec4( near, far, A, - near * A);
    proj_ = glm::perspective( fov_, ratio, near, far);
    bUseOrtho_ = false;
  }

  // void set_ortho(float w, float h, float near, float far) {
  //   width_  = w;
  //   height_ = h;
  //   //linear_params_ = glm::vec4( near, far, 1.0, 1.0); //
  //   float k = 1.f/200.0f;
  //   proj_ = glm::ortho(-w*k, w*k, -h*k, h*k, near, 10.f*far); 
  //   bUseOrtho_ = true;
  // }

  void update(float dt) {
    if (controller_) {
      controller_->update(dt);
      controller_->get_view_matrix(glm::value_ptr(view_));
      world_    = glm::inverse(view_);
      viewproj_ = proj_ * view_;
    }
  }

  inline ViewController *controller() { return controller_; }
  inline ViewController const* controller() const { return controller_; }

  inline float fov() const noexcept { return fov_; }

  inline int width() const noexcept { return width_; }
  inline int height() const noexcept { return height_; }
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

  inline glm::vec3 target() const noexcept { return controller_->target(); }

  inline glm::vec3 pos_ws() const noexcept  { return glm::vec3(view_[3]); } // xxx

  inline bool is_ortho() const noexcept { return bUseOrtho_; }

 private:
  ViewController *controller_;

  int width_;
  int height_;
  float fov_;
  glm::vec4 linear_params_;

  glm::mat4 view_;
  glm::mat4 world_;
  glm::mat4 proj_;
  glm::mat4 viewproj_;

  bool bUseOrtho_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_CORE_CAMERA_H_
