#ifndef SHADER_IM3D_INC_COMMON_GLSL_
#define SHADER_IM3D_INC_COMMON_GLSL_

// ----------------------------------------------------------------------------
// @note All the shaders from the shaders/im3d directory are adapted from 
//        the im3d OpenGL 3.3 example from John Chapman.
// ----------------------------------------------------------------------------

#define VertexData \
  _VertexData { \
    noperspective float m_edgeDistance; \
    noperspective float m_size; \
    smooth vec4 m_color; \
  }

#define kAntialiasing 2.0

// ----------------------------------------------------------------------------

#endif // SHADER_IM3D_INC_COMMON_GLSL_