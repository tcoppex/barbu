#include "ui/imgui_wrapper.h"
#include <cstdint>

// ----------------------------------------------------------------------------

namespace imgui_utils {

void display_texture(unsigned int tex_id, float width, float height) {
  auto const id = (void*)(intptr_t)(tex_id);
  ImGui::Image(id, ImVec2(width, height), ImVec2(0, 0), ImVec2(1,-1));
}

} // namespace imgui_utils

// ----------------------------------------------------------------------------