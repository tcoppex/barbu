#ifndef SHADER_INC_LIGHTING_GLSL_
#define SHADER_INC_LIGHTING_GLSL_

// ----------------------------------------------------------------------------

struct Light {
  vec4 position;    // XYZ Position + W Type
  vec4 color;       // XYZ RGB Color + W intensity
  vec4 direction;
  vec4 params;
};

// ----------------------------------------------------------------------------

// layout(set = 0, binding = 2) uniform LightsInfo
// {
//   uint  count;
//   Light light[MAX_LIGHT_COUNT];
// } lights;

// ----------------------------------------------------------------------------

vec3 apply_diffuse_light(in vec4 color, in vec3 world_to_light, in vec3 normal) {
  return color.w * color.rgb * clamp(dot(world_to_light, normal), 0.0, 1.0);
}

vec3 apply_directional_light(in Light light, in vec3 normal) {
  const vec3 world_to_light = - normalize(light.direction.xyz);
  return apply_diffuse_light(light.color, world_to_light, normal);
}

vec3 apply_point_light(in Light light, in vec3 pos, in vec3 normal) {
  const float kMaxDist = 200.0; //
  const float kMaxDistSquared = kMaxDist * kMaxDist;

  // World light position.
  const vec3 lightWS = light.position.xyz - pos;

  // Calculate attenuation.
  const float light_d2 = dot(lightWS, lightWS);
  const float atten = smoothstep(0.0, 1.0, kMaxDistSquared / light_d2);

  // Normalize vector to light.
  const vec3 to_light = lightWS * inversesqrt(light_d2);
  
  return atten * apply_diffuse_light( light.color, to_light, normal);
}

// ----------------------------------------------------------------------------

#endif // SHADER_INC_LIGHTING_GLSL_
