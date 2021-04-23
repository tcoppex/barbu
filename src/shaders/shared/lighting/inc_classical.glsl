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

// Simple Lambert diffuse model.
vec3 apply_light(in vec3 N, in vec3 L, in vec3 color) {
  return color * max(0.0, dot(N, L));
}

// Diffuse and specular model.
vec3 apply_light(in vec3 N, in vec3 L, in vec3 R, in vec3 V, in vec3 color, float exponent) {
  const float NdotL = dot(N, L);
  const float RdotV = dot(R, V);

  const float Kd = max(0.0, NdotL);
  const float Ks = pow(max(0.0, RdotV), exponent) * (1.0 - step(NdotL, 0.0));
  
  const vec3 lighting = color * (Kd + Ks);

  return clamp(lighting, vec3(0), vec3(1));
}

vec3 blinn(in vec3 N, in vec3 L, in vec3 H, in vec3 color, float exponent) {
  return apply_light( N, L, N, H, color, exponent);
}

vec3 phong(in vec3 N, in vec3 L, in vec3 V, in vec3 color, float exponent) {
  const vec3 R = reflect(N, L);
  return apply_light( N, L, R, V, color, exponent);
}

// ----------------------------------------------------------------------------

vec3 apply_directional_light(in Light light, in vec3 N) {
  const vec3 L = - normalize(light.direction.xyz);
  return apply_light( N, L, light.color.rgb)
       * light.color.a
       ;
}

vec3 apply_point_light(in Light light, in vec3 pos, in vec3 N, float radius) {
  const float kMaxDistSquared = radius * radius;

  // World light position.
  const vec3 lightWS = light.position.xyz - pos;

  // Calculate attenuation.
  const float lds = dot(lightWS, lightWS);
  const float atten = (lds > kMaxDistSquared) ? smoothstep(0.0, 1.0, kMaxDistSquared / lds) : 1.0;

  // Normalize vector to light.
  const vec3 L = lightWS * inversesqrt(lds);
  
  return apply_light( N, L, light.color.rgb)
       * light.color.a
       * atten 
       ;
}

vec3 apply_point_light(in Light light, in vec3 pos, in vec3 N) {
  return apply_point_light(light, pos, N, 200.0);
}

// ----------------------------------------------------------------------------

#endif // SHADER_INC_LIGHTING_GLSL_
