#ifndef ARCBALL_CONTROLLER_H_
#define ARCBALL_CONTROLLER_H_

#include <cmath>
#include "core/camera.h"
#include "core/events.h"
#include "core/graphics.h"

#include "glm/gtc/matrix_transform.hpp"

#ifndef ABC_USE_CUSTOM_TARGET
#define ABC_USE_CUSTOM_TARGET   1
#endif

// ----------------------------------------------------------------------------

//
// Orbital ViewController for a camera around the Y axis / XZ plane and with 3D panning.
//
class ArcBallController : public Camera::ViewController {
 public:
  ArcBallController()
    : last_mouse_x_(0.0),
      last_mouse_y_(0.0),
      yaw_(0.0),
      yaw2_(0.0),
      pitch_(0.0),
      pitch2_(0.0),
      dolly_(0.0),
      dolly2_(0.0)
  {}

  void update(float dt) final {
    TEventData const e = GetEventData();

    update(
      (double)dt,
      e.bMouseMove, 
      e.bMiddleMouse || e.bLeftAlt,
      e.bRightMouse || e.bLeftShift,
      (double)e.mouseX, (double)e.mouseY, (double)e.wheelDelta
    );

    // Keypad quick views.
    auto const pi      = M_PI;
    auto const half_pi = pi / 2;
    auto const rshift  = half_pi / 4;
    auto const rx      = yaw2_;
    auto const ry      = pitch2_; 

    // Check that the last char was entered with the keypad.
    // [Should be rewritten to test true keysym against GLFW_KEY_KP_*]
    if (!e.bKeypad) {
      return;
    }

    switch (e.lastChar) {
      // "Default" view.
      case GLFW_KEY_0:
        reset_target();
        set_view(pi / 16.0, pi / 8.0);
        set_dolly(6.0);
      break;

      // Side axis views.
      case GLFW_KEY_1:
        set_view(0.0, 0.0);
        //reset_target();
        bSideViewSet_ = true;
      break;

      case GLFW_KEY_3:
        set_view(0.0, -half_pi);
        //reset_target();
        bSideViewSet_ = true;
      break;

      case GLFW_KEY_7:
        set_view(half_pi, 0.0);
        //reset_target();
        bSideViewSet_ = true;
      break;

      // Quick axis rotations.
      case GLFW_KEY_2:
        set_view(rx - rshift, ry);
      break;

      case GLFW_KEY_4:
        set_view(rx, ry + rshift);
      break;

      case GLFW_KEY_6:
        set_view(rx, ry - rshift);
      break;

      case GLFW_KEY_8:
        set_view(rx + rshift, ry);
      break;

      // Reverse Y-axis view.
      case GLFW_KEY_9:
        set_view(rx, ry + pi, kDefaultSmoothTransition, false);
      break;

      default:
      break;
    }
  } 

  void get_view_matrix(float *m) final {
    #if ABC_USE_CUSTOM_TARGET
    // This matrix will orbit around the front of the camera.

    auto const id = glm::mat4(1.0f);
    auto const X = glm::vec3(1.0f, 0.0f, 0.0f);
    auto const Y = glm::vec3(0.0f, 1.0f, 0.0f);

    auto const dolly = glm::vec3( 0.0f, 0.0f, - float(dolly_));

    auto const Tdolly = glm::translate( id, dolly);
    auto const Tpan = glm::translate( id, target_);
    auto const Rx = glm::rotate( id, yawf(), X);
    auto const Ry = glm::rotate( id, pitchf(), Y);
    
    Rmatrix_ = Rx * Ry;

    auto view = Tdolly * Rx * Ry * Tpan;
    memcpy(m, glm::value_ptr(view), sizeof(float) * 16);
  
    #else
    // This matrix will always orbit around the space center.

    //   view.setToIdentity();
    //   view.translate(tx_, ty_, -dolly_);
    //   view.rotate(camera_.yaw,    1.0f, 0.0f, 0.0f);
    //   view.rotate(camera_.pitch,  0.0f, 1.0f, 0.0f);
    
    float const cy = cos(yaw_);
    float const sy = sin(yaw_);
    float const cp = cos(pitch_);
    float const sp = sin(pitch_);

    m[ 0] = cp;
    m[ 1] = sy*sp;
    m[ 2] = -sp*cy;
    m[ 3] = 0.0f;

    m[ 4] = 0.0f;
    m[ 5] = cy;
    m[ 6] = sy;
    m[ 7] = 0.0f;

    m[ 8] = sp;
    m[ 9] = -sy*cp;
    m[10] = cy*cp;
    m[11] = 0.0f;

    m[12] = target_.x;
    m[13] = target_.y;
    m[14] = (float)-dolly_;
    m[15] = 1.0f;

    #endif
  } 

  // --------------------

  inline
  void update(double const deltatime,
             bool const bMoving,
             bool const btnTranslate,
             bool const btnRotate,
             double const mouseX,
             double const mouseY,
             double const wheelDelta) 
  {
    if (bMoving) {
      eventMouseMoved(btnTranslate, btnRotate, mouseX, mouseY);
    }
    eventWheel(wheelDelta);
    smoothTransition(deltatime);
    RegulateAngle(pitch_, pitch2_);
    RegulateAngle(yaw_, yaw2_);
  }

  inline double yaw() const { return yaw_; }
  inline double pitch() const { return pitch_; }
  inline double dolly() const { return dolly_; }


  inline float yawf() const { return static_cast<float>(yaw()); }
  inline float pitchf() const { return static_cast<float>(pitch()); }

  inline void set_yaw(double const value, bool const bNoSmooth=kDefaultSmoothTransition) {
    yaw2_ = value;
    yaw_  = (bNoSmooth) ? value : yaw_; 
  }

  inline void set_pitch(double const value, bool const bNoSmooth=kDefaultSmoothTransition, bool const bFastTarget=kDefaultFastestPitchAngle) {
    // use the minimal angle to target.
    double const v1 = value-kAngleModulo;
    double const v2 = value+kAngleModulo;
    double const d0 = std::abs(pitch_ - value);
    double const d1 = std::abs(pitch_ - v1);
    double const d2 = std::abs(pitch_ - v2);
    double const v = (((d0 < d1) && (d0 < d2)) || !bFastTarget) ? value : (d1 < d2) ? v1 : v2;

    pitch2_ = v;
    pitch_  = (bNoSmooth) ? v : pitch_;
  }

  inline void set_dolly(double const value, bool const bNoSmooth=false) { 
    dolly2_ = value; 
    if (bNoSmooth) {
      dolly_ = dolly2_;
    }
  }

  //---------------
  // [ target is is inversed internally, so we change the sign to compensate externally.. fixme]
  inline glm::vec3 target() const final { 
    return -target_; // 
  }

  inline void set_target(glm::vec3 const& target, bool const bNoSmooth=false) {
    target2_ = -target;
    if (bNoSmooth) {
      target_ = target2_;
    }
  }
  //---------------

  inline void reset_target() {
    target_ = glm::vec3(0.0);
    target2_ = glm::vec3(0.0);
  }

  inline void set_view(double const rx, double const ry, bool const bNoSmooth=kDefaultSmoothTransition, bool const bFastTarget=kDefaultFastestPitchAngle) {
    set_yaw(rx, bNoSmooth);
    set_pitch(ry, bNoSmooth, bFastTarget);
  }

  inline bool is_sideview_set() const { return bSideViewSet_; }

 private:
  // Keep the angles pair into a range specified by kAngleModulo to avoid overflow.
  static void RegulateAngle(double &current, double &target) {
    if (fabs(target) >= kAngleModulo) {
      auto const dist = target - current;
      target = fmod(target, kAngleModulo);
      current = target - dist;
    }
  }

  void eventMouseMoved(bool const btnTranslate,
                       bool const btnRotate,
                       double const mouseX,
                       double const mouseY) 
  {
    auto const dv_x = mouseX - last_mouse_x_;
    auto const dv_y = mouseY - last_mouse_y_;
    last_mouse_x_ = mouseX;
    last_mouse_y_ = mouseY;

    if ((std::abs(dv_x) + std::abs(dv_y)) < kRotateEpsilon) {
      return;
    }

    if (btnRotate) {
      pitch2_ += dv_x * kMouseRAcceleration; //
      yaw2_   += dv_y * kMouseRAcceleration; //
      bSideViewSet_ = false;
    }

    if (btnTranslate) {
      auto const acc = dolly2_ * kMouseTAcceleration; //
      
      #if ABC_USE_CUSTOM_TARGET
      target_ += glm::vec3(glm::vec4( float(acc * dv_x), float(-acc * dv_y), 0.0f, 0.0f) * Rmatrix_);
      target2_ = target_;
      #else
      target2_.x += static_cast<float>(dv_x * acc);
      target2_.y -= static_cast<float>(dv_y * acc);
      #endif
    }
  }

  void eventWheel(double const dx) {
    auto const sign = (fabs(dx) > 1.e-5) ? ((dx > 0.0) ? -1.0 : 1.0) : 0.0;
    dolly2_ *= (1.0 + sign * kMouseWAcceleration); //
  }

  void smoothTransition(double const deltatime) {
    // should filter / bias the final signal as it will keep a small jittering aliasing value
    // due to temporal composition, or better yet : keep an anim timecode for smoother control.
    auto k = kSmoothingCoeff * deltatime;
    k = (k > 1.0) ? 1.0 : k;
    auto constexpr lerp = [](auto a, auto b, auto x) { return a + x * (b-a); };
    yaw_   = lerp(yaw_, yaw2_, k);
    pitch_ = lerp(pitch_, pitch2_, k);
    dolly_ = lerp(dolly_, dolly2_, k);
    target_ = glm::mix(target_, target2_, k);
  }


 private:
  static double constexpr kRotateEpsilon          = 1.0e-7;

  // Angles modulo value to avoid overflow [should be TwoPi cyclic].
  static double constexpr kAngleModulo            = glm::two_pi<double>();

  // Arbitrary damping parameters (Rotation, Translation, and Wheel / Dolly).
  static double constexpr kMouseRAcceleration     = 0.00208;
  static double constexpr kMouseTAcceleration     = 0.00110;
  static double constexpr kMouseWAcceleration     = 0.150;
  
  // Used to smooth transition, to factor with deltatime.
  static double constexpr kSmoothingCoeff         = 12.0;

  static bool constexpr kDefaultSmoothTransition  = false;
  static bool constexpr kDefaultFastestPitchAngle = true;

  double last_mouse_x_;
  double last_mouse_y_;
  double yaw_, yaw2_;
  double pitch_, pitch2_;
  double dolly_, dolly2_;

// -------------
  glm::vec3 target_  = glm::vec3(0.0f); //
  glm::vec3 target2_ = glm::vec3(0.0f); //

#if ABC_USE_CUSTOM_TARGET
  // [we could avoid keeping the previous rotation matrix].
  glm::mat4 Rmatrix_ = glm::mat4(1.0f);
#endif
// -------------

  bool bSideViewSet_ = false;
};

// ----------------------------------------------------------------------------

#endif // ARCBALL_CONTROLLER_H_
