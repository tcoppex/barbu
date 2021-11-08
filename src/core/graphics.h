#ifndef BARBU_CORE_GRAPHICS_H_
#define BARBU_CORE_GRAPHICS_H_

// ----------------------------------------------------------------------------
//
// Wraps calls to the underlying graphics API and acts as the general
// graphic context using the namespace "gx".
//
// It is also the entrypoint file to access the window manager (glfw) and 
// the graphics API extensions.
//
// (This is a work in progress as some raw OpenGL calls are still in used).
//
// ----------------------------------------------------------------------------

#include <string_view>

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "core/impl/opengl/opengl.h"
#include "core/window.h" // [tmp : for WindowHandle]

// ----------------------------------------------------------------------------

namespace gx {

enum class State {
  Blend,
  CullFace,
  DepthTest,
  ScissorTest,
  StencilTest,
  CubeMapSeamless,
  ProgramPointSize,
  RasterizerDiscard,
  kCount
};

enum class Face {
  Front,
  Back,
  FrontAndBack,
  kCount
};

/* Not available in GLES */
enum class RenderMode {
  Point, 
  Line,
  Fill,
  kCount
};

enum class BlendFactor {
  Zero,
  One,
  
  SrcColor, 
  OneMinusSrcColor, 
  
  DstColor, 
  OneMinusDstColor, 
  
  SrcAlpha,
  OneMinusSrcAlpha, 
  
  DstAlpha, 
  OneMinusDstAlpha,
  
  ConstantColor, 
  OneMinusConstantColor, 
  
  ConstantAlpha,
  OneMinusConstantAlpha,

  kCount
};

enum SamplerName {
  NearestClamp,
  NearestRepeat,
  
  LinearClamp,
  LinearRepeat,

  LinearMipmapClamp,
  LinearMipmapRepeat,

  kNumSamplerName,
  kDefaultSampler = LinearMipmapRepeat
};

// ----------------------------------------------------------------------------

void Initialize(WindowHandle window);
void Deinitialize();

// Pipeline -------------------------------------------------------------------

void Enable(State cap);
void Disable(State cap);

bool IsEnabled(State cap);

void Viewport(int32_t x, int32_t y, int32_t w, int32_t h);
void Viewport(int32_t w, int32_t h);

void BlendFunc(BlendFactor src_factor, BlendFactor dst_factor);

void ClearColor(glm::vec4 const& rgba, bool bGammaCorrect = false);
void ClearColor(glm::vec3 const& rgb, bool bGammaCorrect = false);

void ClearColor(float r, float g, float b, float a = 1.0f, bool bGammaCorrect = false);
void ClearColor(float c, bool bGammaCorrect = false);

void ClearColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff, bool bGammaCorrect = false);
void ClearColor(uint8_t c, bool bGammaCorrect = false);

void CullFace(Face mode);

void DepthMask(bool state);

void LineWidth(float width); //

void PolygonMode(Face face, RenderMode mode);

template<typename T>
T Get(uint32_t pname);// { return 0u; }

template<uint32_t>
uint32_t Get(uint32_t pname);

// Texture  -------------------------------------------------------------------

void BindSampler(int unit, SamplerName name = SamplerName::kDefaultSampler);
void UnbindSampler(int unit);

void BindTexture(uint32_t tex, int unit = 0, SamplerName name = SamplerName::kDefaultSampler);
void UnbindTexture(int unit = 0);

// Program --------------------------------------------------------------------

void UseProgram(uint32_t pgm = 0u);

void LinkProgram(uint32_t pgm);

int32_t UniformLocation(uint32_t pgm, std::string_view name);

int32_t AttribLocation(uint32_t pgm, std::string_view name);

template<typename T>
void SetUniform(uint32_t pgm, int32_t loc, T const& value);

template<typename T>
void SetUniform(uint32_t pgm, std::string_view name, T const& value) {
  if (auto loc = UniformLocation( pgm, name); loc > -1) {
    SetUniform<T>( pgm, loc, value);
  }
}

template<typename T>
void SetUniform(uint32_t pgm, int32_t loc, T const* value, int32_t n);

template<typename T>
void SetUniform(uint32_t pgm, std::string_view name, T const* value, int32_t n) {
  if (auto loc = UniformLocation( pgm, name); loc > -1) {
    SetUniform<T>( pgm, loc, value, n);
  }
}

// Return a kernel's required grid dimensions given its parameters.
constexpr uint32_t GetKernelGridDim(int numCells, int blockDim) {
  return static_cast<uint32_t>((numCells + blockDim - 1) / blockDim);
}

template<int tX = 1, int tY = 1, int tZ = 1>
void DispatchCompute(int x = 1, int y = 1, int z = 1) {
 glDispatchCompute(
    GetKernelGridDim(x, tX), 
    GetKernelGridDim(y, tY), 
    GetKernelGridDim(z, tZ)
  ); 
}

// Error ----------------------------------------------------------------------

bool CheckFramebufferStatus();

// (move to their specialized resource / asset class ?)
bool CheckShaderStatus(uint32_t shader, std::string_view name);
bool CheckProgramStatus(uint32_t program, std::string_view name);

void CheckError(std::string_view errMsg, char const* file, int line);

} // namespace gx

// ----------------------------------------------------------------------------

#ifdef NDEBUG
# define CHECK_GX_ERROR()
#else
# define CHECK_GX_ERROR(tString)    gx::CheckError( "" # tString, __FILE__, __LINE__)
#endif

// ----------------------------------------------------------------------------

#endif // BARBU_CORE_GRAPHICS_H_
