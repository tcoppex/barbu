#ifndef AER_CORE_IMPL_DESKTOP_SYMBOLS_H_
#define AER_CORE_IMPL_DESKTOP_SYMBOLS_H_

/* -------------------------------------------------------------------------- */

// [ work in progress ]
// This will be completely rewritten.

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

/* -------------------------------------------------------------------------- */

namespace symbols {

struct Keyboard {
  enum : uint16_t {
    Space       = GLFW_KEY_SPACE,

    a           = 'a',
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i,
    j,
    k,
    l,
    m,
    n,
    o,
    p,
    q,
    r,
    s,
    t,
    u,
    v,
    w,
    x,
    y,
    z,

    Num0        = GLFW_KEY_0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,

    A           = GLFW_KEY_A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    Escape      = GLFW_KEY_ESCAPE,
    Return      = GLFW_KEY_ENTER,
    Tab         = GLFW_KEY_TAB,
    BackSpace   = GLFW_KEY_BACKSPACE,
    Insert      = GLFW_KEY_INSERT,
    Delete      = GLFW_KEY_DELETE,

    Right       = GLFW_KEY_RIGHT,
    Left        = GLFW_KEY_LEFT,
    Down        = GLFW_KEY_DOWN,
    Up          = GLFW_KEY_UP,

    PageUp      = GLFW_KEY_PAGE_UP,
    PageDown    = GLFW_KEY_PAGE_DOWN,
    Home        = GLFW_KEY_HOME,
    End         = GLFW_KEY_END,

    Pause       = GLFW_KEY_PAUSE,

    F1          = GLFW_KEY_F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,

    KP0         = GLFW_KEY_KP_0,
    KP1,
    KP2,
    KP3,
    KP4,
    KP5,
    KP6,
    KP7,
    KP8,
    KP9,
    
    // LControl    = GLFW_KEY_Control_L,
    // RControl    = GLFW_KEY_Control_R,
    // LShift      = GLFW_KEY_Shift_L,
    // RShift      = GLFW_KEY_Shift_R,
    // LAlt        = GLFW_KEY_Alt_L,
    // RAlt        = GLFW_KEY_Alt_R,
    // LSuper      = GLFW_KEY_Super_L,
    // RSuper      = GLFW_KEY_Super_R,

    kCount
  };
};


struct Mouse {
  enum  : uint8_t {
    Left        = GLFW_MOUSE_BUTTON_1,
    Right       = GLFW_MOUSE_BUTTON_2,
    Middle      = GLFW_MOUSE_BUTTON_3,
    XButton1,
    XButton2,
    
    kCount
  };
};

struct SixAxis {
  enum : uint8_t {
    Select      = 0,
    LStickDown  = 1,
    RStickDown  = 2,
    Start       = 3,
    Up          = 4,
    Right       = 5,
    Down        = 6,
    Left        = 7,
    L2          = 8,
    R2          = 9,
    L1          = 10,
    R1          = 11,
    Triangle    = 12,
    Circle      = 13,
    Cross       = 14,
    Square      = 15,
    PSButton    = 16,

    kCount
  };
};

}  // namespace symbols

#endif  // AER_CORE_IMPL_DESKTOP_SYMBOLS_H_
