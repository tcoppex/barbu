#include "ui/views/Main.h"
#include "ui/imgui_wrapper.h"

#include "core/display.h"

#ifndef UI_DBG_STR
#ifdef NDEBUG
  #define UI_DBG_STR ""
#else
  #define UI_DBG_STR " [Debug ON]" 
#endif
#endif // UI_DBG_STR

namespace views {

void Main::render() {
  // TODO : switch for absolution coordinates.
  ImGui::SetNextWindowPos(
    ImVec2(8 * DEBUG_HDPI_SCALING, 8 * DEBUG_HDPI_SCALING)
  );
  ImGui::SetNextWindowSize(
    ImVec2(450 * DEBUG_HDPI_SCALING, 900 * DEBUG_HDPI_SCALING)
  );

  auto flags = ImGuiWindowFlags_NoMove 
             // | ImGuiWindowFlags_NoBringToFrontOnFocus 
             // | ImGuiWindowFlags_NoNavFocus 
             // | ImGuiWindowFlags_NoFocusOnAppearing
             ;
             
  if (!ImGui::Begin("Parameters" UI_DBG_STR, nullptr, flags)) {
    ImGui::End();
    return;
  }

  ImGui::Spacing();
  ImGui::TextWrapped("Welcome to barbü, an hair simulation & rendering playground.");
  ImGui::Spacing();

  ImGui::Text("Here some basic inputs :");
  ImGui::BulletText("Right-click + mouse to orbit.");
  ImGui::BulletText("Middle-click + mouse to pan.");
  ImGui::BulletText("Scroll to dolly.");
  ImGui::BulletText("Escape to quit. Oh no !");

  // (might change for an overlay dialog)
  if (ImGui::TreeNode("(more inputs)")) {
    ImGui::BulletText("[a] select / unselect all.");
    ImGui::BulletText("[h] to toggle UI.");
    ImGui::BulletText("[w] to toggle wireframe.");
    ImGui::BulletText("Drag-n-drop to import OBJ / GLTF.");
    ImGui::Spacing();
    ImGui::Text("When entities are selected : ");
    ImGui::BulletText("[r] to rotate.");
    ImGui::BulletText("[s] to scale.");
    ImGui::BulletText("[t] to translate.");
    ImGui::BulletText("double [r]/[t] to switch space.");
    ImGui::BulletText("[LeftCtrl] to snap.");
    ImGui::BulletText("[x] to reset transform.");
    ImGui::BulletText("[X] to delete.");
    ImGui::Spacing();
    ImGui::Text("View control : ");
    ImGui::BulletText("[c] to focus on centroid.");
    ImGui::BulletText("[C] to focus on pivot.");
    ImGui::BulletText("[j] focus on next entity.");
    ImGui::BulletText("[k] focus on previous entity.");
    ImGui::BulletText("Keypad 0 to reset view.");
    ImGui::BulletText("Keypad 1/3/7 to side view.");
    ImGui::BulletText("Keypad 2/4/6/8 to quick orbit.");
    ImGui::BulletText("Keypad 9 to invert view.");
    ImGui::TreePop();
  }

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();

  // Sub views.
  ImGui::PushItemWidth(160);
  for (auto view : views_) {
    view->render();
    ImGui::Spacing();
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // Framerate info.
  {
    float const ms = 1000.0f / ImGui::GetIO().Framerate;
    int const fps = static_cast<int>(ImGui::GetIO().Framerate);

    ImGui::Text("%.3f ms/frame (%d FPS)", (double)ms, fps);
    ImGui::SameLine();
    ImGui::Checkbox("", &params_.regulate_fps);
  }

  //ImGui::Checkbox("Use postprocessing", &params_.postprocess);

  ImGui::End();
}

}  // namespace views
