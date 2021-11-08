#ifndef BARBU_UI_VIEWS_SCENE_HIERARCHY_H_
#define BARBU_UI_VIEWS_SCENE_HIERARCHY_H_

#include <algorithm>
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
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
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


  inline bool isSelected(int32_t index) const {
    return (index < static_cast<int32_t>(selected_.size())) ? selected_[index] : false;
  }

  inline void select(int32_t index, bool status) {
    assert(index < static_cast<int32_t>(selected_.size()));
    selected_[index] = status;
  }
  
  inline void selectAll(bool status) {
    std::fill(selected_.begin(), selected_.end(), status);
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
    node_flags |= (isSelected(i) ? ImGuiTreeNodeFlags_Selected : 0);

    // Automatically open in-between nodes.
    if (!children.empty()) {
      //ImGui::SetNextItemOpen(true, ImGuiCond_Once);
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

 public:
  std::vector<bool> selected_; // should be in SceneHierarchy
};

}  // namespace views

#endif  // BARBU_UI_VIEWS_SCENE_HIERARCHY_H_
