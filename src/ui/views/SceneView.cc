#include "ui/views/SceneView.h"
#include "ui/imgui_wrapper.h"

// ----------------------------------------------------------------------------

namespace views {

void SceneView::render() {
  if (!ImGui::CollapsingHeader("Scene")) {
    return;
  }
    
  // Scene parameters.
  if (ImGui::TreeNode("General")) {
    ImGui::Checkbox("Show gizmo", &params_.show_transform);
    ImGui::Checkbox("Show grid", &params_.show_grid);
    ImGui::Checkbox("Show skybox", &params_.show_skybox);
    //ImGui::Checkbox("Show wireframe (w)", &params_.show_wireframe);
    ImGui::Checkbox("Show hair", &params_.enable_hair);
    ImGui::Checkbox("Show particle", &params_.enable_particle);
    //ImGui::Checkbox("Center pivot", &params_.center_pivot);
    //ImGui::Checkbox("Smooth lines", &params_.smoothline);
    //ImGui::Checkbox("Freeze", &params_.freeze);
  
    ImGui::TreePop();
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Scene hierarchy.
  if (params_.ui_view_ptr) {
    params_.ui_view_ptr->render();
  }

}

}  // namespace "views"

// ----------------------------------------------------------------------------
