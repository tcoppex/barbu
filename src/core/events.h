#ifndef BARBU_CORE_EVENTS_H_
#define BARBU_CORE_EVENTS_H_

#include <cstdint>
#include <vector>
#include <string>
struct GLFWwindow;

// ----------------------------------------------------------------------------

struct TEventData {
  float wheelDelta;
  float mouseX;
  float mouseY;
  bool bMouseMove;
  bool bLeftMouse;
  bool bRightMouse;
  bool bMiddleMouse;
  bool bSpacePressed;
  bool bLeftCtrl;
  bool bLeftAlt;
  bool bLeftShift;
  bool bKeypad;
  uint16_t lastChar;

  std::vector<std::string> dragFilenames;
};

extern void InitEvents(GLFWwindow* window);
extern void HandleEvents();
extern TEventData const GetEventData();

// ----------------------------------------------------------------------------

#endif // BARBU_CORE_EVENTS_H_
