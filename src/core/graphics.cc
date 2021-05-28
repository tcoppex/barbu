#include "core/graphics.h"

#include <array>
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "core/logger.h"
#include "memory/enum_array.h"

// ----------------------------------------------------------------------------
// Load Extensions.
// ----------------------------------------------------------------------------

#ifndef USE_GLEW

GLFWglproc getAddress(char const* name) {
  GLFWglproc ptr = glfwGetProcAddress(name);
  if (nullptr == ptr) {
    LOG_ERROR("Extension function \"", name, "\" was not found.");
  }
  return ptr;
}

/* Automatically generated pointers to extension's function */
#include "core/ext/_extensions.inl"

#endif  // USE_GLEW

// ----------------------------------------------------------------------------
// Local utility functions.
// ----------------------------------------------------------------------------

namespace {

// Return an OpenGL error enum as a string.
char const* GetErrorString(GLenum err) {
#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif
  switch (err) {
    case GL_NO_ERROR:
      return STRINGIFY(GL_NO_ERROR);
      
    case GL_INVALID_ENUM:
      return STRINGIFY(GL_INVALID_ENUM);
      
    case GL_INVALID_VALUE:
      return STRINGIFY(GL_INVALID_VALUE);
      
    case GL_INVALID_OPERATION:
      return STRINGIFY(GL_INVALID_OPERATION);
      
    case GL_STACK_OVERFLOW:
      return STRINGIFY(GL_STACK_OVERFLOW);
      
    case GL_STACK_UNDERFLOW:
      return STRINGIFY(GL_STACK_UNDERFLOW);
      
    case GL_OUT_OF_MEMORY:
      return STRINGIFY(GL_OUT_OF_MEMORY);
      
    default:
      return "GetErrorString : Unknown constant";
  }
#undef STRINGIFY
}

// Return true if all extensions are supported, false otherwhise.
bool checkExtensions(char const* extensions[]) {
  bool valid = true;
  for (int i = 0; extensions && (extensions[i] != nullptr); ++i) {
    if (!glfwExtensionSupported(extensions[i])) {
      LOG_WARNING("Extension \"", extensions[i], "\" is not supported.");
      valid = false;
    }
  }
  return valid;
}

static constexpr int32_t kErrorBufferSize = 1024;
static char s_errorBuffer[kErrorBufferSize]; //

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
  GL_TEXTURE_CUBE_MAP_SEAMLESS,
  GL_PROGRAM_POINT_SIZE,
  
  //GL_LINE_SMOOTH, // [do not use]
};

static const
EnumArray<uint32_t, Face> gl_facemode{
  GL_FRONT,
  GL_BACK,
  GL_FRONT_AND_BACK,
};

static const
EnumArray<uint32_t, RenderMode> gl_polygonmode{
  GL_POINT, 
  GL_LINE,
  GL_FILL,
};

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

  std::array<Params_t, kNumSamplerName> params{
    GL_NEAREST,               GL_NEAREST,   GL_CLAMP_TO_EDGE,
    GL_NEAREST,               GL_NEAREST,   GL_REPEAT,
    GL_LINEAR,                GL_LINEAR,    GL_CLAMP_TO_EDGE,
    GL_LINEAR,                GL_LINEAR,    GL_REPEAT,
    GL_LINEAR_MIPMAP_LINEAR,  GL_LINEAR,    GL_CLAMP_TO_EDGE,
    GL_LINEAR_MIPMAP_LINEAR,  GL_LINEAR,    GL_REPEAT,
  };

  glCreateSamplers( kNumSamplerName, sSamplers.data());

  for (int i = 0; i < kNumSamplerName; ++i) {
    auto const id = sSamplers[i];
    auto const p  = params[i];

    glSamplerParameteri( id, GL_TEXTURE_MIN_FILTER, p.min_filter);
    glSamplerParameteri( id, GL_TEXTURE_MAG_FILTER, p.mag_filter);
    glSamplerParameteri( id, GL_TEXTURE_WRAP_S,     p.wrap);
    glSamplerParameteri( id, GL_TEXTURE_WRAP_T,     p.wrap);
    glSamplerParameteri( id, GL_TEXTURE_WRAP_R,     p.wrap);

    glSamplerParameterf( id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f); //
  }
  LOG_DEBUG_INFO( "All samplers have anisotropy set to 8.0" );

  //atexit([](){gx::Deinitialize();});

  CHECK_GX_ERROR();
}

}  // namespace gx


// ----------------------------------------------------------------------------
// Graphic context wrapper.
// ----------------------------------------------------------------------------

namespace gx {

void Initialize() {
  char const* s_extensions[]{
    "GL_ARB_compute_shader",
    "GL_ARB_seamless_cubemap_per_texture",
    "GL_ARB_separate_shader_objects",
    "GL_ARB_shader_image_load_store",
    "GL_ARB_shader_storage_buffer_object",
    "GL_EXT_texture_filter_anisotropic",
    nullptr
  };

  // Check if specific extensions exists.
  checkExtensions(s_extensions);

#ifdef USE_GLEW
  // Load GLEW.
  glewExperimental = GL_TRUE;
  GLenum result = glewInit();

  // flush doubtful error.
  glGetError();

  if (GLEW_OK != result) {
    LOG_ERROR( glewGetErrorString(result) );
  }
#else
  // Load function pointers.
  LoadExtensionFuncPtrs();
#endif

  InitializeSamplers();
}

void Deinitialize() {
  glDeleteSamplers( kNumSamplerName, sSamplers.data());
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

void Viewport(float x, float y, float w, float h) {
  glViewport( x, y, w, h);
}

void Viewport(float w, float h) {
  Viewport( 0.0f, 0.0f, w, h);
}

void BlendFunc(BlendFactor src_factor, BlendFactor dst_factor) {
  glBlendFunc( gl_blendfactor[src_factor], gl_blendfactor[dst_factor] );
}


// Note : for proper internal gamma correction, we should use a custom
//        color structure instead.

void ClearColor(glm::vec4 const& rgba, bool bGammaCorrect) {
  auto c = (bGammaCorrect) ? glm::vec4(glm::pow( glm::vec3(rgba), glm::vec3(2.2f)), rgba.w) : rgba;
  glClearColor(c.x, c.y, c.z, c.w);
}

void ClearColor(glm::vec3 const& rgb, bool bGammaCorrect) {
  ClearColor( glm::vec4(rgb, 1.0f), bGammaCorrect);
}

void ClearColor(float r, float g, float b, float a, bool bGammaCorrect) {
  ClearColor(glm::vec4(r, g, b, a), bGammaCorrect);
}

void ClearColor(int8_t r, int8_t g, int8_t b, int8_t a, bool bGammaCorrect) {
  float constexpr s = 1.0f / 255.0f;
  ClearColor( r * s, g * s, b * s, a * s, bGammaCorrect);
}

void ClearColor(float c, bool bGammaCorrect) {
  ClearColor( glm::vec3(c), bGammaCorrect);
}

void ClearColor(int8_t c, bool bGammaCorrect) {
  ClearColor( c, c, c, 0xff, bGammaCorrect);
}


void CullFace(Face mode) {
  glCullFace( gl_facemode[mode] );
}

void DepthMask(bool state) {
  glDepthMask( state ? GL_TRUE : GL_FALSE );
}

void LineWidth(float width) {
  assert( "glLineWidth is inconsistent. Do not use it ヽ(￣～￣　)ノ" && 0 );
  //glLineWidth( width );
}

void PolygonMode(Face face, RenderMode mode) {
  glPolygonMode( gl_facemode[face], gl_polygonmode[mode] );
}

template<>
uint32_t Get(uint32_t pname) {
  GLint v;
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
  glBindTextureUnit(unit, tex);
  BindSampler(unit, name);
}

void UnbindTexture(int unit) {
  glBindTextureUnit(unit, 0);
  UnbindSampler(unit);
}

void UseProgram(uint32_t pgm) {
  glUseProgram(pgm);
}

void LinkProgram(uint32_t pgm) {
  glLinkProgram(pgm);
}

int32_t UniformLocation(uint32_t pgm, std::string_view name) {
  GLint loc = glGetUniformLocation(pgm, name.data());
#ifndef NDEBUG
  if (loc < 0) {
    // TODO : retrieve program's fullname from manager.
    LOG_WARNING( "Uniform missing :", name );
  }
#endif
  return loc;
}

int32_t AttribLocation(uint32_t pgm, std::string_view name) {
  GLint loc = glGetAttribLocation(pgm, name.data());
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
  GLint status = 0;

  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    glGetShaderInfoLog(shader, kErrorBufferSize, nullptr, s_errorBuffer);
    LOG_ERROR( name, "\n", s_errorBuffer);
    return false;
  }

  return true;
}

bool CheckProgramStatus(uint32_t program, std::string_view name) {
  GLint status = 0;

  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    glGetProgramInfoLog(program, kErrorBufferSize, nullptr, s_errorBuffer);
    LOG_ERROR( name, "\n", s_errorBuffer );
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
  GLenum const err = glGetError();
  if (err != GL_NO_ERROR) {
    auto const error_string = GetErrorString(err);
    if (msg != nullptr) {
      Logger::Get().fatal_error( file, line, "OpenGL", msg, "[", error_string, "]");
    } else {
      Logger::Get().fatal_error( file, line, "OpenGL [", error_string, "]");
    }
  }
}

}  // namespace gx

// ----------------------------------------------------------------------------*