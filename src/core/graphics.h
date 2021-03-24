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

#include <cassert>
#include <string_view>
#include "core/logger.h"

// extensions -----------------------------------------------------------------

#ifdef USE_GLEW
#include "GL/glew.h"
#else
#include "core/ext/_extensions.h"
#endif

// window manager -------------------------------------------------------------

#include "GLFW/glfw3.h"

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
  kCount
};

enum class Face {
  Front,
  Back,
  FrontAndBack,
  kCount
};

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


void Initialize();
void Deinitialize();

// Pipeline -------------------------------------------------------------------

void Enable(State cap);
void Disable(State cap);

void BlendFunc(BlendFactor src_factor, BlendFactor dst_factor);

void ClearColor(float r, float g, float b, float a = 1.0f);
void ClearColor(int32_t r, int32_t g, int32_t b, int32_t a = 255);

//void Clear();

void CullFace(Face mode);

void DepthMask(bool state);

void LineWidth(float width);

void PolygonMode(Face face, RenderMode mode);

template<typename T>
T Get(uint32_t pname);// { return 0u; }

template<uint32_t>
uint32_t Get(uint32_t pname);

// Texture  -------------------------------------------------------------------

void BindSampler(int image_unit, SamplerName name = SamplerName::kDefaultSampler);
void UnbindSampler(int image_unit);

void BindTexture(uint32_t tex, int image_unit = 0, SamplerName name = SamplerName::kDefaultSampler);
void UnbindTexture(int image_unit = 0);

// Program --------------------------------------------------------------------

void UseProgram(uint32_t pgm = 0u);

int32_t UniformLocation(uint32_t pgm, std::string_view name);

int32_t AttribLocation(uint32_t pgm, std::string_view name);

template<typename T>
void SetUniform( uint32_t pgm, int32_t loc, T const& value);

template<typename T>
void SetUniform( uint32_t pgm, std::string_view name, T const& value) {
  int32_t const loc = UniformLocation( pgm, name);
  if (loc > -1) {
    SetUniform<T>( pgm, loc, value);
  }
}

template<typename T>
void SetUniform( uint32_t pgm, int32_t loc, T const* value, int32_t n);

template<typename T>
void SetUniform( uint32_t pgm, std::string_view name, T const* value, int32_t n) {
  int32_t const loc = UniformLocation( pgm, name);
  if (loc > -1) {
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
# define CHECK_GX_ERROR(s)    gx::CheckError( #s, __FILE__, __LINE__)
#endif

// ----------------------------------------------------------------------------

#endif // BARBU_CORE_GRAPHICS_H_
