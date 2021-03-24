#ifndef BARBU_UI_VIEWS_SCENE_HIERARCHY_H_
#define BARBU_UI_VIEWS_SCENE_HIERARCHY_H_

#include <vector>
#include "ui/ui_view.h"
#include "ui/imgui_wrapper.h"

class SceneHierarchy;

namespace views {

class SceneHierarchyView : public ParametrizedUIView<SceneHierarchy> {
 public:
  SceneHierarchyView(TParameters &params)
    : ParametrizedUIView(params)
  {}

  void render() override {
    ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
    if (!ImGui::TreeNode("Hierarchy")) {
      return;
    }

    // reset parameters.
    node_clicked_ = -1;
    {
      size_t const size_select = selected_.size();
      selected_.resize( params_.entities_.size(), false);

      // Select only the very last addition (even with group import), except first one.
      if ((size_select > 0) && (size_select < selected_.size())) {
        selected_.assign(selected_.size(), false);
        selected_.back() = true;
      }
    }
    
    // Render the hierarchy node tree.
    for (auto child : params_.root_->children()) {
      render_sub_hierarchy(child);
    }

    // Manage selection.
    // Ctrl+click for multiselection, single click for singular selection. 
    if (node_clicked_ >= 0) {
      bool const sel = !selected_[node_clicked_];
      if (!ImGui::GetIO().KeyCtrl) {
        selected_.assign(selected_.size(), false);
      }
      selected_[node_clicked_] = sel;
    }

    ImGui::TreePop();
  }

  inline bool is_selected(int32_t index) const {
    return (index < static_cast<int32_t>(selected_.size())) ? selected_[index] : false;
  }

 private:
  void render_sub_hierarchy(EntityHandle entity) {
    assert( nullptr != entity );

    auto const& children = entity->children();
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow 
                                  | ImGuiTreeNodeFlags_OpenOnDoubleClick
                                  ;
    // Leaf flags.
    if (children.empty()) {
      node_flags |= ImGuiTreeNodeFlags_Leaf 
                  | ImGuiTreeNodeFlags_NoTreePushOnOpen
                  | ImGuiTreeNodeFlags_Bullet
                  ;
    }

    char const* name = entity->name().c_str();    
    int32_t const i  = entity->index(); //

    // Add selected flag.
    node_flags |= (is_selected(i) ? ImGuiTreeNodeFlags_Selected : 0);

    // Automatically open in-between nodes.
    if (!children.empty()) {
      ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
    }

    auto node_open = ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, "%s", name);
    node_clicked_  = (ImGui::IsItemClicked()) ? i : node_clicked_;

    // Process node children.
    if (!children.empty() && node_open) {
      for (auto child : children) {
        render_sub_hierarchy(child);
      }
      ImGui::TreePop();
    }
  }

  int32_t node_clicked_;
  std::vector<bool> selected_;
};

}  // namespace views

#endif  // BARBU_UI_VIEWS_SCENE_HIERARCHY_H_
