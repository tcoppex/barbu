#ifndef BARBU_CORE_GLFW_H_
#define BARBU_CORE_GLFW_H_

/* Wrapper around glfw to always load OpenGL extensions before it. */

// extensions -----------------------------------------------------------------

#ifdef USE_GLEW
#include "GL/glew.h"
#else
#include "core/ext/_extensions.h"
#endif

// window manager -------------------------------------------------------------

#include "GLFW/glfw3.h"

// ----------------------------------------------------------------------------

#endif  // BARBU_CORE_GLFW_H_