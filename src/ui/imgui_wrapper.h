#ifndef BARBU_UI_IMGUI_WRAPPER_H_
#define BARBU_UI_IMGUI_WRAPPER_H_

// ----------------------------------------------------------------------------

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wconversion"
#endif // __GNUC__

#include "imgui/imgui.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

// ----------------------------------------------------------------------------

namespace imgui_utils {
  void display_texture(unsigned int tex_id, float width, float height);
}

// ----------------------------------------------------------------------------

#endif // BARBU_UI_IMGUI_WRAPPER_H_