#version 430 core

// ----------------------------------------------------------------------------

layout(location = 0) in vec2 inPosition;

uniform mat4x4 uModel;
uniform mat4x4 uViewproj;
uniform float uScaleFactor;
uniform vec4 uColor;
uniform float uGridResolution = 32;

out VDataBlock {
  vec4 color;
} OUT;

// ----------------------------------------------------------------------------

void main() {
  vec3 pos = mat3(uModel) * vec3(inPosition.x, 0.0f, inPosition.y);

  // Main color.
  vec3 col = vec3(lessThan(abs(pos), vec3(0.1)));
  vec3 axis_color = 0.75f*(vec3(1.0) - col);//(vec3(1) - step(abs(pos), vec3(0))) * 0.75f;
  bool is_central_axis = any(equal(inPosition.xy, vec2(0.0f)));

  // Alpha value (to dim sub grid).
  vec2 apos = uGridResolution * abs(inPosition.xy); 
  bool is_major_axis = all(equal(ivec2(mod(apos, 2)), ivec2(0)));
  float alphaFactor = is_major_axis ? 1.0 : 0.25f;

  OUT.color.rgb = (is_central_axis) ? axis_color : uColor.rgb;
  OUT.color.a = alphaFactor * uColor.a;

  gl_Position = uViewproj * vec4(uScaleFactor * pos, 1.0f);
}

// ----------------------------------------------------------------------------
