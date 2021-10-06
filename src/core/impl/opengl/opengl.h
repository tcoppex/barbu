#ifndef BARBU_CORE_IMPL_OPENGL_OPENGL_H_
#define BARBU_CORE_IMPL_OPENGL_OPENGL_H_

#if !defined(__ANDROID__)

#ifdef USE_GLEW
#include "GL/glew.h"
#else
#include "core/impl/opengl/_extensions.h"
#endif

#else // defined(__ANDROID__)

#if __ANDROID_API__ >= 24
#include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21
#include <GLES3/gl31.h>
#else
#include <GLES3/gl3.h>
#endif

#endif // !defined(__ANDROID__)

#if !defined(GL_ES_VERSION_3_0) && !defined(GL_VERSION_4_2)
#error "Minimum header version must be OpenGL ES 3.0 or OpenGL 4.2"
#endif

#endif // BARBU_CORE_IMPL_OPENGL_OPENGL_H_
