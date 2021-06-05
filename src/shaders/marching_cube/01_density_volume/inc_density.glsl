#include "shared/inc_maths.glsl"
#include "shared/inc_distance_utils.glsl"
#include "shared/perlin/inc_perlin_3d.glsl"

uniform float uTime = 1.0;

// ----------------------------------------------------------------------------

float terrain(in vec3 ws) {
  float d = 0.0f;

  //d = sdPlane(ws, vec4(0.0f, 1.0f, 0.0f, 0.0f));

  const float scale = 5.0f;
  const float radius = 20.0f;
  const float noise  = fbm_3d(ws * scale);
  const float noise2 = fbm_3d(ws);

  d += 2.f * noise / scale;
  ws += 0.05f*noise; 
  d = opIntersection(d, sdSphere(ws, radius));
  
  const float d2 = sdTorus(ws + 0.25f*noise2, vec2(1.2f*radius, 1.0f));
  d = opSmoothUnion(d, d2, 0.75f);

  return d;
}

// ----------------------------------------------------------------------------

float compute_density(in vec3 ws) {
  float d = 0.0f;

#if 1
  d = terrain(ws);
#else
  //ws = ws * rotX(radians(90.0f));
  //d += sdPlane(ws, vec4(0.0f, 1.0f, 0.0f, 0.0f));
  //d += sdCylinder(ws-vec3(0.0f), 1.0f);
  d += sdTorus(ws + sin(PiOverTwo()*uTime*0.1)*0.25f*fbm_3d(ws+uTime), vec2(20.0f, 1.0f));

  ws *= 1.0f / 2.5f;

  float dx = 10.5f;
  vec3 ws2 = ws + fbm_3d(dx*ws + uTime) / 55.f;

  ws2 += opDisplacement(ws, 0.7f);
  //ws2 = opRepeat(ws, vec3(16.0f + 0.2f*fbm_3d(ws)));

  float d2 = 0.0f;
  d2 = sdSphere(ws2 + vec3(0.0f,0.f,+7.0f), 7.0f);

  //d = opUnion(d, d2);
  d = opSmoothUnion(d, d2, 0.75f);
#endif

  return d;
}

// ----------------------------------------------------------------------------