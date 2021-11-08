#ifndef BARBU_UI_UI_CONTROLLER_H_
#define BARBU_UI_UI_CONTROLLER_H_

#include <memory>

struct ImDrawData;
class UIView;
class AbstractWindow; //

// ----------------------------------------------------------------------------

class UIController {
 public:
  UIController() :
    mainview_ptr_(nullptr)
  {}

  void init(/*GLFWwindow* window*/);
  void deinit();

  // [should not sent the (abstract) window but its Events / View ]
  void update(std::shared_ptr<AbstractWindow> window); 
  
  void render(bool show_ui = true);

  inline void set_mainview(std::shared_ptr<UIView> view) {
    mainview_ptr_ = view;
  }

 private:
  void create_device_objects();
  void destroy_device_objects();
  void create_font_texture();
  void render_frame(ImDrawData* draw_data);

  //GLFWwindow *window_ptr_;
  std::shared_ptr<UIView> mainview_ptr_;

  struct TDeviceObjects {
    unsigned int fontTexture = 0;
    unsigned int shaderHandle = 0;
    unsigned int vertHandle = 0;
    unsigned int fragHandle = 0;
    int uTex = 0;
    int uProjMtx = 0;
    unsigned int aPosition = 0;
    unsigned int aUV = 0;
    unsigned int aColor = 0;
    unsigned int vboHandle = 0;
    unsigned int elementsHandle = 0;
  } device_;

 private:
  UIController(UIController const&) = delete;
  UIController(UIController&&) = delete;
};

// ----------------------------------------------------------------------------

#endif // BARBU_UI_UI_CONTROLLER_H_
