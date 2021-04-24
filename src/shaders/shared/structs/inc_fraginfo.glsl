#ifndef SHADERS_SHARED_STRUCTS_INC_FRAGINFO_GLSL_
#define SHADERS_SHARED_STRUCTS_INC_FRAGINFO_GLSL_

struct FragInfo_t {
  vec3 P;           // position
  vec3 N;           // normal
  vec3 V;           // viewdir
  vec2 uv;          // texcoord
  float n_dot_v;    // saturate( dot(n, v) )
};

#endif // SHADERS_SHARED_STRUCTS_INC_FRAGINFO_GLSL_