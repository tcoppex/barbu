#include "core/graphics.h"

#include <array>
#include <algorithm>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "core/logger.h"
#include "core/window.h"
#include "memory/enum_array.h"

// ----------------------------------------------------------------------------
// Load Extensions.
// ----------------------------------------------------------------------------

#if !defined(USE_GLEW) && !defined(__ANDROID__)

/* Redefines GLEXTGEN macro to catch missing extensions. */
#ifndef NDEBUG
#define GLEXTGEN_GET_PROC_ADDRESS(tName) \
  ([] (auto *addr) { \
    if (!addr) { \
      LOG_ERROR( "Extension function \"", tName, "\" was not found." ); \
    } \
    return addr; \
  })(getProcAddress(tName))
#endif

/* Automatically generated pointers to extensions's entrypoints. */
#include "core/impl/opengl/_extensions.inl"

#endif  // USE_GLEW && __ANDROID__

// ----------------------------------------------------------------------------
// Local utility functions.
// ----------------------------------------------------------------------------

namespace {

// [move] Default gamma correction factor.
constexpr float kGammaFactor{2.2f};
constexpr glm::vec3 kGammaRGBFactor{kGammaFactor};

// Loading screen clear color. Zoidberg-ish-red.
constexpr glm::vec3 kDefaultScreenCleanColor{ 0.75f, 0.27f, 0.23f };

// Buffer for errors retrieval.
static constexpr int32_t kErrorBufferSize{1024};
static std::array<char, kErrorBufferSize> s_errorBuffer{}; //

#ifndef GX_STRINGIFY
#define GX_STRINGIFY(x) #x
#endif

// Return an OpenGL error enum as a string.
char const* GetErrorString(GLenum err) {
  switch (err) {
    case GL_NO_ERROR:
      return GX_STRINGIFY(GL_NO_ERROR);
      
    case GL_INVALID_ENUM:
      return GX_STRINGIFY(GL_INVALID_ENUM);
      
    case GL_INVALID_VALUE:
      return GX_STRINGIFY(GL_INVALID_VALUE);
      
    case GL_INVALID_OPERATION:
      return GX_STRINGIFY(GL_INVALID_OPERATION);
      
    case GL_STACK_OVERFLOW:
      return GX_STRINGIFY(GL_STACK_OVERFLOW);
      
    case GL_STACK_UNDERFLOW:
      return GX_STRINGIFY(GL_STACK_UNDERFLOW);
      
    case GL_OUT_OF_MEMORY:
      return GX_STRINGIFY(GL_OUT_OF_MEMORY);
      
    default:
      return "GetErrorString : Unknown constant";
  }
}

#undef GX_STRINGIFY

} // namespace


// ----------------------------------------------------------------------------
// Internal data.
// ----------------------------------------------------------------------------

namespace gx {

static const
EnumArray<uint32_t, State> gl_capability{
  GL_BLEND,
  GL_CULL_FACE,
  GL_DEPTH_TEST,
  GL_SCISSOR_TEST,
  GL_STENCIL_TEST,

#ifndef GL_ES_VERSION_2_0
  GL_TEXTURE_CUBE_MAP_SEAMLESS, // [!GLes]
  GL_PROGRAM_POINT_SIZE,        // [!GLes]
#else
  std::numeric_limits<uint32_t>::max(),
  std::numeric_limits<uint32_t>::max(),
#endif

  GL_RASTERIZER_DISCARD,
  
  //GL_LINE_SMOOTH, // [do not use]
};

#ifndef GL_ES_VERSION_2_0
 // [!GLes]
static const
EnumArray<uint32_t, Face> gl_facemode{
  GL_FRONT,
  GL_BACK,
  GL_FRONT_AND_BACK,
};

 // [!GLes]
static const
EnumArray<uint32_t, RenderMode> gl_polygonmode{
  GL_POINT, 
  GL_LINE,
  GL_FILL,
};
#endif // GL_ES_VERSION_2_0

static const
EnumArray<uint32_t, BlendFactor> gl_blendfactor{
  GL_ZERO, 
  GL_ONE, 
  GL_SRC_COLOR, 
  GL_ONE_MINUS_SRC_COLOR, 
  GL_DST_COLOR, 
  GL_ONE_MINUS_DST_COLOR, 
  GL_SRC_ALPHA,
  GL_ONE_MINUS_SRC_ALPHA, 
  GL_DST_ALPHA,
  GL_ONE_MINUS_DST_ALPHA,
  GL_CONSTANT_COLOR, 
  GL_ONE_MINUS_CONSTANT_COLOR, 
  GL_CONSTANT_ALPHA, 
  GL_ONE_MINUS_CONSTANT_ALPHA
};

static 
std::array<uint32_t, kNumSamplerName> sSamplers;

// Initialize the default samplers.
void InitializeSamplers() {
  struct Params_t {
    GLenum min_filter;
    GLenum mag_filter;
    GLenum wrap;
  };

  std::array<Params_t, kNumSamplerName> params{{
    { GL_NEAREST,               GL_NEAREST,   GL_CLAMP_TO_EDGE },
    { GL_NEAREST,               GL_NEAREST,   GL_REPEAT },
    { GL_LINEAR,                GL_LINEAR,    GL_CLAMP_TO_EDGE },
    { GL_LINEAR,                GL_LINEAR,    GL_REPEAT },
    { GL_LINEAR_MIPMAP_LINEAR,  GL_LINEAR,    GL_CLAMP_TO_EDGE },
    { GL_LINEAR_MIPMAP_LINEAR,  GL_LINEAR,    GL_REPEAT },
  }};

#ifdef GL_ES_VERSION_2_0
  glGenSamplers( kNumSamplerName, sSamplers.data());
#else
  glCreateSamplers( kNumSamplerName, sSamplers.data());
#endif

  for (int i = 0; i < kNumSamplerName; ++i) {
    auto const id = sSamplers[i];
    auto const p  = params[i];

    glSamplerParameteri( id, GL_TEXTURE_MIN_FILTER, p.min_filter);
    glSamplerParameteri( id, GL_TEXTURE_MAG_FILTER, p.mag_filter);
    glSamplerParameteri( id, GL_TEXTURE_WRAP_S,     p.wrap);
    glSamplerParameteri( id, GL_TEXTURE_WRAP_T,     p.wrap);
    glSamplerParameteri( id, GL_TEXTURE_WRAP_R,     p.wrap);

#ifndef GL_ES_VERSION_2_0
    glSamplerParameterf( id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f); //
    LOG_DEBUG_INFO( "All samplers have anisotropy set to 8.0" );
#endif
  }

  CHECK_GX_ERROR();
}

}  // namespace gx


// ----------------------------------------------------------------------------
// Graphic context wrapper.
// ----------------------------------------------------------------------------

namespace gx {

void Initialize(WindowHandle window) {
  // Retrieve generic info.
  {
    char const* const vendor   = (char const*)glGetString(GL_VENDOR);
    char const* const renderer = (char const*)glGetString(GL_RENDERER);
    char const* const version  = (char const*)glGetString(GL_VERSION);
    char const* const shader   = (char const*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    LOG_INFO( "Vendor :", vendor);
    LOG_INFO( "Renderer :", renderer);
    LOG_INFO( "Version :", version);
    LOG_INFO( "Shader :", shader);
  }

  // Extensions utils functions from the Window Manager (platform specific).
  auto extLoaderFuncs{ window->getExtensionLoaderFuncs() };

  // Check if specific extensions exists.
  if (extLoaderFuncs.isExtensionSupported) {
    std::vector<const char*> extensions{
      "GL_ARB_bindless_texture",
      "GL_ARB_compute_shader",
      "GL_ARB_gl_spirv",
      "GL_ARB_gpu_shader5",
      "GL_ARB_seamless_cubemap_per_texture",
      "GL_ARB_separate_shader_objects",
      "GL_ARB_shader_image_load_store",
      "GL_ARB_shader_storage_buffer_object",
      "GL_EXT_texture_filter_anisotropic",
      "GL_EXT_texture_sRGB"

      // "GL_ARB_ES3_1_compatibility",
      // "GL_ARB_ES3_2_compatibility", 
    };
    std::for_each( extensions.cbegin(), extensions.cend(), [&](auto const &ext) {
      if (!extLoaderFuncs.isExtensionSupported(ext)) {
        LOG_WARNING( "Extension \"", ext, "\" is not supported." );
      }
    });
  } else {
    LOG_ERROR( "Cannot check extensions support." );
  }

  // Load extensions functions pointers.
#if !defined(__ANDROID__)
#if defined(USE_GLEW)
  glewExperimental = GL_TRUE;
  if (GLenum result = glewInit(); GLEW_OK != result) {
    LOG_ERROR( glewGetErrorString(result) );
  }
  // flush out doubtful error.
  glGetError();
#else
  if (extLoaderFuncs.getProcAddress) {
    glextgen_LoadExtensionFuncPtrs( extLoaderFuncs.getProcAddress );
  } else {
    LOG_ERROR( "Cannot load extensions functions." );
  }
#endif // USE_GLEW
#endif // !__ANDROID__

  // Initialize custom samplers.
  InitializeSamplers();
  
  // PreClean the screen.
  {
    gx::Viewport(window->width(), window->height());
    gx::ClearColor(kDefaultScreenCleanColor);
    glClear( GL_COLOR_BUFFER_BIT ); //
    window->flush();
  }
}

void Deinitialize() {
  // Wait for the device to finish all its commands.
  glFinish();

  if (sSamplers[0] != 0) {
    glDeleteSamplers( kNumSamplerName, sSamplers.data());
  }
}

void Enable(State cap) {
  glEnable( gl_capability[cap] );
}

void Disable(State cap) {
  glDisable( gl_capability[cap] );
}

bool IsEnabled(State cap) {
  return glIsEnabled( gl_capability[cap] );
}

void Viewport(int32_t x, int32_t y, int32_t w, int32_t h) {
  glViewport( x, y, w, h);
}

void Viewport(int32_t w, int32_t h) {
  Viewport( 0, 0, w, h);
}

void BlendFunc(BlendFactor src_factor, BlendFactor dst_factor) {
  glBlendFunc( gl_blendfactor[src_factor], gl_blendfactor[dst_factor] );
}

// Note : for proper internal gamma correction, we should use a custom
//        color structure instead.

void ClearColor(glm::vec4 const& rgba, bool bGammaCorrect) {
  auto c{ ([&](auto const& rgb) { 
    return bGammaCorrect ? glm::pow( rgb, kGammaRGBFactor) : rgb; 
  })(glm::vec3(rgba)) };
  glClearColor(c.x, c.y, c.z, rgba.w);
}

void ClearColor(glm::vec3 const& rgb, bool bGammaCorrect) {
  ClearColor( glm::vec4(rgb, 1.0f), bGammaCorrect);
}

void ClearColor(float r, float g, float b, float a, bool bGammaCorrect) {
  ClearColor(glm::vec4(r, g, b, a), bGammaCorrect);
}

void ClearColor(float r, float g, float b, bool bGammaCorrect) {
  ClearColor(glm::vec4(r, g, b, 1.0f), bGammaCorrect);
}

void ClearColor(float c, bool bGammaCorrect) {
  ClearColor( glm::vec3(c), bGammaCorrect);
}

void ClearColor(int32_t r, int32_t g, int32_t b, int32_t a, bool bGammaCorrect) {
  float constexpr s{ 1.0f / 255.0f };
  ClearColor( (r & 0xff) * s, (g & 0xff) * s, (b & 0xff) * s, (a & 0xff) * s, bGammaCorrect);
}

void ClearColor(int32_t r, int32_t g, int32_t b, bool bGammaCorrect) {
  ClearColor( r, g, b, 0xff, bGammaCorrect);
}

// void ClearColor(int32_t c, bool bGammaCorrect) {
//   ClearColor( c, c, c, 0xff, bGammaCorrect);
// }

void CullFace(Face mode) {
#ifndef GL_ES_VERSION_2_0  
  glCullFace( gl_facemode[mode] );
#endif // GL_ES_VERSION_2_0
}

void DepthMask(bool state) {
  glDepthMask( state ? GL_TRUE : GL_FALSE );
}

void LineWidth(float width) {
  LOG_ERROR( "glLineWidth is deprecated (not forward compatible). Do not use it ヽ(￣～￣　)ノ" );
  //glLineWidth( width );
}

void PolygonMode(Face face, RenderMode mode) {
#ifndef GL_ES_VERSION_2_0  
  glPolygonMode( gl_facemode[face], gl_polygonmode[mode] );
#endif // GL_ES_VERSION_2_0
}

template<>
uint32_t Get(uint32_t pname) {
  GLint v{0};
  glGetIntegerv(pname, &v);
  return static_cast<uint32_t>(v);
}

void BindSampler(int unit, SamplerName name) {
  glBindSampler(unit, sSamplers[name]);
}

void UnbindSampler(int unit) {
  glBindSampler(unit, 0);
}

void BindTexture(uint32_t tex, int unit, SamplerName name) {
#ifndef GL_ES_VERSION_2_0
  glBindTextureUnit(unit, tex);
#else
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture( GL_TEXTURE_2D, tex); //
  glActiveTexture(GL_TEXTURE0);
#endif
  BindSampler(unit, name);
}

void UnbindTexture(int unit) {
#ifndef GL_ES_VERSION_2_0
  glBindTextureUnit(unit, 0);
#else
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture( GL_TEXTURE_2D, 0); //
  glActiveTexture(GL_TEXTURE0);
#endif
  UnbindSampler(unit);
}

void UseProgram(uint32_t pgm) {
  glUseProgram(pgm);
}

void LinkProgram(uint32_t pgm) {
  glLinkProgram(pgm);
}

int32_t UniformLocation(uint32_t pgm, std::string_view name) {
  int32_t const loc{ glGetUniformLocation(pgm, name.data()) };
#ifndef NDEBUG
  if (loc < 0) {
    // TODO : retrieve program's fullname from manager.
    LOG_WARNING( "Uniform missing :", name );
  }
#endif
  return loc;
}

int32_t AttribLocation(uint32_t pgm, std::string_view name) {
  int32_t const loc{ glGetAttribLocation(pgm, name.data()) };
#ifndef NDEBUG
  if (loc < 0) {
    LOG_WARNING( "Attribute missing :", name );
  }
#endif
  return loc;
}


template<>
void SetUniform(uint32_t pgm, int32_t loc, float const& value) {
  glProgramUniform1f( pgm, loc, value);
}

template<>
void SetUniform(uint32_t pgm, int32_t loc, int32_t const& value) {
  glProgramUniform1i( pgm, loc, value);
}

template<>
void SetUniform(uint32_t pgm, int32_t loc, bool const& value) {
  glProgramUniform1i( pgm, loc, value);
}

template<>
void SetUniform(uint32_t pgm, int32_t loc, uint32_t const& value) {
  glProgramUniform1ui( pgm, loc, value);
}

template<>
void SetUniform(uint32_t pgm, int32_t loc, glm::vec2 const& value) {
  glProgramUniform2fv( pgm, loc, 1, glm::value_ptr(value));
}

template<>
void SetUniform(uint32_t pgm, int32_t loc, glm::vec3 const& value) {
  glProgramUniform3fv( pgm, loc, 1, glm::value_ptr(value));
}

template<>
void SetUniform(uint32_t pgm, int32_t loc, glm::vec4 const& value) {
  glProgramUniform4fv( pgm, loc, 1, glm::value_ptr(value));
}

template<>
void SetUniform(uint32_t pgm, int32_t loc, glm::mat3 const& value) {
  glProgramUniformMatrix3fv( pgm, loc, 1, GL_FALSE, glm::value_ptr(value));
}

template<>
void SetUniform(uint32_t pgm, int32_t loc, glm::mat4 const& value) {
  glProgramUniformMatrix4fv( pgm, loc, 1, GL_FALSE, glm::value_ptr(value));
}

template<>
void SetUniform(uint32_t pgm, int32_t loc, glm::mat4 const* value, int32_t n) {
  glProgramUniformMatrix4fv( pgm, loc, n, GL_FALSE, glm::value_ptr(*value));
}


bool CheckFramebufferStatus() {
  return (GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
}

bool CheckShaderStatus(uint32_t shader, std::string_view name){
  GLint status{0};
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

  if (status != GL_TRUE) {
    glGetShaderInfoLog(shader, kErrorBufferSize, nullptr, s_errorBuffer.data());
    LOG_ERROR( name, "\n", s_errorBuffer.data());
    return false;
  }

  return true;
}

bool CheckProgramStatus(uint32_t program, std::string_view name) {
  GLint status{0};
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  
  if (status != GL_TRUE) {
    glGetProgramInfoLog(program, kErrorBufferSize, nullptr, s_errorBuffer.data());
    LOG_ERROR( name, "\n", s_errorBuffer.data() );
  }

  glValidateProgram(program);
  glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
  if (status != GL_TRUE) {
    LOG_ERROR( "Program \"", name, "\" failed to be validated." );
    return false;
  }

  return true;
}

void CheckError(std::string_view msg, char const* file, int line) {
  if (auto const err = glGetError(); err != GL_NO_ERROR) {
    auto const error_string = GetErrorString(err);
    if (msg != nullptr) {
      Logger::Get().fatal_error(file, "", line, "OpenGL", msg, "[", error_string, "]");
    } else {
      Logger::Get().fatal_error(file, "", line, "OpenGL [", error_string, "]");
    }
  }
}

}  // namespace gx

// ----------------------------------------------------------------------------*