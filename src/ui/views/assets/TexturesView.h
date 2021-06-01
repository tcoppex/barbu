#ifndef BARBU_UI_VIEWS_ASSETS_TEXTURES_VIEW_H_
#define BARBU_UI_VIEWS_ASSETS_TEXTURES_VIEW_H_

#include "ui/ui_view.h"
#include "ui/imgui_wrapper.h"
#include "memory/assets/assets.h"

// ----------------------------------------------------------------------------

// ref : https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#about-imtextureid

namespace views {

class TexturesView : public UIView {
 public:
  TexturesView() = default;

  void render() final {
    
    if (!show_window_) {
      return;
    }

    ImGui::Begin("Loaded Textures", &show_window_, ImGuiWindowFlags_AlwaysAutoResize);
    
    // Display all currently loaded 2D textures.
    int index = 1;
    for (auto& tuple : TEXTURE_ASSETS.assets_) {
      auto texture = tuple.second;
      
      if (glIsTexture(texture->id) && (texture->params.target == GL_TEXTURE_2D)) {
        auto const tex_id = reinterpret_cast<ImTextureID>(texture->id);
        ImGui::Image( tex_id, ImVec2(r_,r_));
        if (index % lw_) ImGui::SameLine();
        ++index;
      }
    }

    ImGui::End();
  }

  void show(bool state) {
    show_window_ = state;
  }

  void toggle(bool state=true) {
    show_window_ ^= (show_window_ != state);
  }

 private:
  int32_t r_  = 256;
  int32_t lw_ = 3;
  bool show_window_ = false;
};

}  // namespace "views"

// ----------------------------------------------------------------------------

#endif  // BARBU_UI_VIEWS_ASSETS_TEXTURES_VIEW_H_
