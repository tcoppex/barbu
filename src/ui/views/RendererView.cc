#include "ui/views/RendererView.h"
#include "ui/imgui_wrapper.h"

// ----------------------------------------------------------------------------

namespace views {

void RendererView::render() {
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  
  if (!ImGui::CollapsingHeader("General")) {
    return;
  }
    
  // Rendering parameters.
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::TreeNode("General")) {
    // ImGui::Checkbox("Show gizmo",           &params_.show_transform);
    ImGui::Checkbox("Show grid",            &params_.show_grid);
    ImGui::Checkbox("Show skybox",          &params_.show_skybox);
    ImGui::Checkbox("Show rigs",            &params_.show_rigs);
    // ImGui::Checkbox("Show wireframe (w)",   &params_.show_wireframe);
    ImGui::Checkbox("Show hair",            &params_.enable_hair);
    ImGui::Checkbox("Show particle",        &params_.enable_particle);
  
    ImGui::TreePop();
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Scene hierarchy.
  if (auto ui = params_.sub_view; ui) {
    ui->render();
  }

}

}  // namespace "views"

// ----------------------------------------------------------------------------
