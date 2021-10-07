#version 430 core

// ----------------------------------------------------------------------------

layout(location = 0) in vec2 inPosition;

uniform mat4x4 uModel;
uniform mat4x4 uViewproj;

uniform float uScaleFactor;
uniform float uGridResolution = 32;

// Colors should be premultiplied for gamma correction (default : 2.2)
uniform vec4 uColor;
uniform mat3 uAxisColors = mat3(
  vec3(1.0, 0.167, 0.289), // x-axis
  vec3(0.289, 1.0, 0.167), // y-axis
  vec3(0.167, 0.289, 1.0)  // z-axis
);

out VDataBlock {
  vec4 color;
} OUT;

// ----------------------------------------------------------------------------

void main() {
  vec3 pos = mat3(uModel) * vec3(inPosition.x, 0.0f, inPosition.y);

  // Main color.
  const float eps = 0.01;
  vec3 axis = vec3(greaterThan(abs(pos), vec3(eps)));
  vec3 axis_color = uAxisColors * axis;
  bool is_central_axis = any(equal(inPosition.xy, vec2(0)));

  // Alpha value (to dim sub grid).
  const int subgridStep = 2; // [should be a power of two]
  vec2 apos = uGridResolution * abs(inPosition.xy); 
  bool is_major_axis = all(equal(ivec2(mod(apos, subgridStep)), ivec2(0))) || is_central_axis;
  float alphaFactor = is_major_axis ? 1.0 : 0.33f;

  OUT.color.rgb = is_central_axis ? axis_color : uColor.rgb;
  OUT.color.a = alphaFactor * uColor.a;

  gl_Position = uViewproj * vec4(uScaleFactor * pos, 1.0f);
}

// ----------------------------------------------------------------------------
