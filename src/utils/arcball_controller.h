#ifndef ARCBALL_CONTROLLER_H_
#define ARCBALL_CONTROLLER_H_

#include <cmath>
#include "glm/gtc/matrix_transform.hpp"

#include "core/camera.h"
#include "core/graphics.h"
#include "core/events.h" //

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
    auto const& e{ Events::Get() };

    update( 
      dt, 
      e.mouseMoved(), 
      e.buttonDown(2) || e.keyDown(342),  //   e.bMiddleMouse || e.bLeftAlt,
      e.buttonDown(1) || e.keyDown(340),  //   e.bRightMouse || e.bLeftShift,
      (double)e.mouseX(), (double)e.mouseY(), 
      (double)e.wheelDelta()
    );

    // [fixme]
    /// Event Signal processing.

    // Keypad quick views.
    auto const pi       { M_PI };
    auto const half_pi  { pi / 2 };
    auto const rshift   { half_pi / 4 };
    auto const rx       { yaw2_ };
    auto const ry       { pitch2_ }; 

    // Check that the last char was entered with the keypad.
    // [Should be rewritten to test true keysym against GLFW_KEY_KP_*]
    // if (!e.bKeypad) {
    //   return;
    // }
    switch (e.lastInputChar()) {
      // "Default" view.
      case '0': //GLFW_KEY_0:
        resetTarget();
        setView(pi / 16.0, pi / 8.0);
        setDolly(6.0);
      break;

      // Side axis views.
      case '1': //GLFW_KEY_1:
        setView(0.0, 0.0);
        //resetTarget();
        bSideViewSet_ = true;
      break;

      case '3': //GLFW_KEY_3:
        setView(0.0, -half_pi);
        //resetTarget();
        bSideViewSet_ = true;
      break;

      case '7': //GLFW_KEY_7:
        setView(half_pi, 0.0);
        //resetTarget();
        bSideViewSet_ = true;
      break;

      // Quick axis rotations.
      case '2': //GLFW_KEY_2:
        setView(rx - rshift, ry);
      break;

      case '4': //GLFW_KEY_4:
        setView(rx, ry + rshift);
      break;

      case '6': //GLFW_KEY_6:
        setView(rx, ry - rshift);
      break;

      case '8': //GLFW_KEY_8:
        setView(rx + rshift, ry);
      break;

      // Reverse Y-axis view.
      case '9': //GLFW_KEY_9:
        setView(rx, ry + pi, kDefaultSmoothTransition, false);
      break;

      default:
      break;
    }
  }

  void getViewMatrix(float m[]) final {
    static_assert( sizeof(glm::mat4) == (16*sizeof(float)) );
    #if ABC_USE_CUSTOM_TARGET
    // This matrix will orbit around the front of the camera.

    auto const id = glm::mat4(1.0f);
    auto const X = glm::vec3(1.0f, 0.0f, 0.0f);
    auto const Y = glm::vec3(0.0f, 1.0f, 0.0f);

    auto const dolly = glm::vec3( 0.0f, 0.0f, - static_cast<float>(dolly_));

    auto const Tdolly = glm::translate( id, dolly);
    auto const Tpan = glm::translate( id, target_);
    auto const Rx = glm::rotate( id, yawf(), X);
    auto const Ry = glm::rotate( id, pitchf(), Y);
    
    Rmatrix_ = Rx * Ry;

    auto view{ Tdolly * Rx * Ry * Tpan };
    memcpy(m, glm::value_ptr(view), sizeof view);
  
    #else
    // This matrix will always orbit around the space center.

    //   view.setToIdentity();
    //   view.translate(tx_, ty_, -dolly_);
    //   view.rotate(camera_.yaw,    1.0f, 0.0f, 0.0f);
    //   view.rotate(camera_.pitch,  0.0f, 1.0f, 0.0f);
    
    float const cy{ cos(yaw_) };
    float const sy{ sin(yaw_) };
    float const cp{ cos(pitch_) };
    float const sp{ sin(pitch_) };

    m[ 0] = cp;
    m[ 1] = sy * sp;
    m[ 2] = -sp * cy;
    m[ 3] = 0.0f;

    m[ 4] = 0.0f;
    m[ 5] = cy;
    m[ 6] = sy;
    m[ 7] = 0.0f;

    m[ 8] = sp;
    m[ 9] = -sy * cp;
    m[10] = cy * cp;
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

  inline void setYaw(double const value, bool const bSmooth=kDefaultSmoothTransition) {
    yaw2_ = value;
    yaw_  = (!bSmooth) ? value : yaw_; 
  }

  inline void setPitch(double const value, bool const bSmooth=kDefaultSmoothTransition, bool const bFastTarget=kDefaultFastestPitchAngle) {
    // use the minimal angle to target.
    double const v1{ value - kAngleModulo };
    double const v2{ value + kAngleModulo };
    double const d0{ std::abs(pitch_ - value) };
    double const d1{ std::abs(pitch_ - v1) };
    double const d2{ std::abs(pitch_ - v2) };
    double const v{ (((d0 < d1) && (d0 < d2)) || !bFastTarget) ? value : (d1 < d2) ? v1 : v2 };

    pitch2_ = v;
    pitch_  = (!bSmooth) ? v : pitch_;
  }

  inline void setDolly(double const value, bool const bSmooth=kDefaultSmoothTransition) { 
    dolly2_ = value;
    if (!bSmooth) {
      dolly_ = dolly2_;
    }
  }

  //---------------
  // [ target is is inversed internally, so we change the sign to compensate externally.. fixme]
  inline glm::vec3 target() const final { 
    return -target_; // 
  }

  inline void setTarget(glm::vec3 const& target, bool const bSmooth=kDefaultSmoothTransition) {
    target2_ = -target;
    if (!bSmooth) {
      target_ = target2_;
    }
  }
  //---------------

  inline void resetTarget() {
    target_ = glm::vec3(0.0);
    target2_ = glm::vec3(0.0);
  }

  inline void setView(double const rx, double const ry, bool const bSmooth=kDefaultSmoothTransition, bool const bFastTarget=kDefaultFastestPitchAngle) {
    setYaw(rx, bSmooth);
    setPitch(ry, bSmooth, bFastTarget);
  }

  inline bool isSideView() const { return bSideViewSet_; }

 private:
  // Keep the angles pair into a range specified by kAngleModulo to avoid overflow.
  static void RegulateAngle(double &current, double &target) {
    if (fabs(target) >= kAngleModulo) {
      auto const dist{ target - current };
      target = fmod(target, kAngleModulo);
      current = target - dist;
    }
  }

  void eventMouseMoved(bool const btnTranslate,
                       bool const btnRotate,
                       double const mouseX,
                       double const mouseY) 
  {
    auto const dv_x{ mouseX - last_mouse_x_ };
    auto const dv_y{ mouseY - last_mouse_y_ };
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
      auto const acc{ dolly2_ * kMouseTAcceleration }; //
      
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
    auto const sign{ (fabs(dx) > 1.e-5) ? ((dx > 0.0) ? -1.0 : 1.0) : 0.0 };
    dolly2_ *= (1.0 + sign * kMouseWAcceleration); //
  }

  void smoothTransition(double const deltatime) {
    // should filter / bias the final signal as it will keep a small jittering aliasing value
    // due to temporal composition, or better yet : keep an anim timecode for smoother control.
    auto k{ kSmoothingCoeff * deltatime };
    k = (k > 1.0) ? 1.0 : k;
    auto constexpr lerp{ [](auto a, auto b, auto x) { return a + x * (b-a); }}; //
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

  static bool constexpr kDefaultSmoothTransition  = true;
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
