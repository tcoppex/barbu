#ifndef BARBU_CORE_DISPLAY_H_
#define BARBU_CORE_DISPLAY_H_

// ............................................................................
//
//  The Display structure acts as an interface for window creation, it holds
//  the necessary data to inform how to create a window handle.
//
// [rename "GraphicsEnum.h" or something]
//
// ............................................................................

#include <cstdint>

#ifndef DEBUG_HDPI_SCALING
#define DEBUG_HDPI_SCALING 1.5
#endif

/* -------------------------------------------------------------------------- */

// Integer type describing a displayable surface (aka. a matrices of pixels).
// if non-positive the value will be derived from a maximum resolution.
using SurfaceSize = int16_t;

// Remap a SurfaceSize based on its value and a maximum resolution :
// - a value <= 0 set it to a scale of the maximum resolution for its monitor.
// - otherwise clamp between 1 and the maximum resolution.
inline constexpr SurfaceSize ClampSurfaceSize(SurfaceSize size, SurfaceSize max_size) {
  return ((size > max_size) ? max_size :
                (size <= 0) ? static_cast<SurfaceSize>(0.8 * max_size) //
                            : size) * 
          DEBUG_HDPI_SCALING;
}

/* -------------------------------------------------------------------------- */

// Which graphics API backend to use, this might move elsewhere.
enum class GraphicsAPI : uint8_t {
  kOpenGL     = 0,              //< OpenGL Core 4.2+ / ES 3.0+.
  kVulkan     = 1,              //< Vulkan.
  
  kCount,
  kDefault    = kOpenGL,        //< (Platform dependent).
};

// Level of functionality used by the renderer.
enum class ShaderModel : uint8_t {
  GL_ES_30    = 0,              //< mobile functionality (ShaderModel 4.0)
  GL_CORE_42  = 1,              //< desktop functionality (ShaderModel 5.0)
  
  kCount
};

/* -------------------------------------------------------------------------- */

// Features of the window enabled at creation.
enum DisplayFlags : uint16_t { //
  DisplayFlags_None        = 0,

  DisplayFlags_FullScreen  = 1 << 0,
  DisplayFlags_Resizable   = 1 << 1,
  DisplayFlags_Decorated   = 1 << 2,
  
  DisplayFlags_Default     = DisplayFlags_Resizable | DisplayFlags_Decorated,
};

// The Display structure holds the properties used to create a display context.
struct Display {
  SurfaceSize    width        = -1;
  SurfaceSize    height       = -1;
  int32_t        msaa_samples = 4; //
  DisplayFlags   flags        = DisplayFlags_Default;
  GraphicsAPI    api          = GraphicsAPI::kDefault; //
  ShaderModel    shader_model = ShaderModel::GL_CORE_42; //
  //int8_t         screen_id = 0;
};

/* -------------------------------------------------------------------------- */

#endif  // BARBU_CORE_DISPLAY_H_
