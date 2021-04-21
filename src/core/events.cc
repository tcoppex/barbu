#include "events.h"

#include <cstring>
#include "core/glfw.h"
#include "ui/imgui_wrapper.h"

// ----------------------------------------------------------------------------

/* Event data accessed by callbacks */
static TEventData s_Global;

// ----------------------------------------------------------------------------

namespace {

void keyboard_cb(GLFWwindow *window, int key, int scancode, int action, int mods);

void char_cb(GLFWwindow*, unsigned int c);

void mouse_button_cb(GLFWwindow* window, int button, int action, int mods);

void mouse_move_cb(GLFWwindow* window, double xpos, double ypos);

void scroll_cb(GLFWwindow* window, double x, double y);

void drop_cb(GLFWwindow* window, int count, const char** paths);

} // namespace

// ----------------------------------------------------------------------------

extern
void InitEvents(GLFWwindow* window) {
  s_Global.wheelDelta = 0.0f;
  s_Global.mouseX = 0.0f;
  s_Global.mouseY = 0.0f;
  s_Global.bMouseMove = false;
  s_Global.bLeftMouse = false;
  s_Global.bRightMouse = false;
  s_Global.bMiddleMouse = false;
  s_Global.bSpacePressed = false;
  s_Global.bLeftCtrl = false;
  s_Global.bLeftAlt = false;
  s_Global.bLeftShift = false;
  s_Global.bKeypad = false;
  s_Global.dragFilenames.clear();

  glfwSetKeyCallback(window, keyboard_cb);
  glfwSetCharCallback(window, char_cb);
  glfwSetMouseButtonCallback(window, mouse_button_cb);
  glfwSetCursorPosCallback(window, mouse_move_cb);
  glfwSetScrollCallback(window, scroll_cb);
  glfwSetDropCallback(window, drop_cb);
}

extern
void HandleEvents() {
  // reset per-frame events.
  s_Global.bMouseMove = false;
  s_Global.wheelDelta = 0.0f;
  s_Global.dragFilenames.clear();
  s_Global.lastChar = -1;

  glfwPollEvents();
}

extern
TEventData const GetEventData() {
  return s_Global;
}

// ----------------------------------------------------------------------------

namespace {

void UpdateButton(int key, int action, int id, bool &btn) {
  btn = btn || ((key == id) && (action == GLFW_PRESS));
  btn = btn && ((key == id) && (action != GLFW_RELEASE));
}

void keyboard_cb(GLFWwindow *window, int key, int, int action, int) {
  if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS)) {
    glfwSetWindowShouldClose(window, 1);
  }

  if ((key == GLFW_KEY_SPACE) && (action == GLFW_PRESS)) {
    s_Global.bSpacePressed ^= true;
  }

  // [temporary]
  if ((key >= GLFW_KEY_KP_0) && (key <= GLFW_KEY_KP_9)) {
    s_Global.bKeypad |= (action == GLFW_PRESS);
    s_Global.bKeypad &= (action != GLFW_RELEASE);
  }

  // When the UI capture keyboard, don't process it.
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard || io.WantCaptureMouse) {
    io.KeysDown[key] = (action == GLFW_PRESS);

    io.KeyCtrl  = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT]   || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt   = io.KeysDown[GLFW_KEY_LEFT_ALT]     || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER]   || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
    
    //return;
  }

  UpdateButton(key, action, GLFW_KEY_LEFT_CONTROL, s_Global.bLeftCtrl);
  UpdateButton(key, action, GLFW_KEY_LEFT_ALT,      s_Global.bLeftAlt);
  UpdateButton(key, action, GLFW_KEY_LEFT_SHIFT,  s_Global.bLeftShift);
}

void char_cb(GLFWwindow*, unsigned int c) {
  if ((c > 0) && (c < 0x10000))
  {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
      io.AddInputCharacter(c);
      
      if (io.WantCaptureMouse) {
        return;
      }
    }

    s_Global.lastChar = static_cast<unsigned short>(c);
  }
}

void mouse_button_cb(GLFWwindow*, int button, int action, int) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  UpdateButton( button, action, GLFW_MOUSE_BUTTON_LEFT, s_Global.bLeftMouse);
  UpdateButton( button, action, GLFW_MOUSE_BUTTON_RIGHT, s_Global.bRightMouse);
  UpdateButton( button, action, GLFW_MOUSE_BUTTON_MIDDLE, s_Global.bMiddleMouse);
}

void mouse_move_cb(GLFWwindow*, double xpos, double ypos) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }
  s_Global.mouseX = static_cast<float>(xpos);
  s_Global.mouseY = static_cast<float>(ypos);
  s_Global.bMouseMove = true;
}

void scroll_cb(GLFWwindow*, double x, double y) {
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    io.MouseWheelH += static_cast<float>(x);
    io.MouseWheel += static_cast<float>(y);
    return;
  }
  s_Global.wheelDelta = 0.5f * static_cast<float>(y);
}

void drop_cb(GLFWwindow*, int count, const char** paths) {
  //strcpy(s_Global.dragFilename, paths[0]);
  for (int i=0; i<count; ++i) {
    s_Global.dragFilenames.push_back( std::string(paths[i]) );
  }
}

} // namespace

// ----------------------------------------------------------------------------
