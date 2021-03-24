#version 430 core

// ----------------------------------------------------------------------------

layout(location = 0) in vec2 inPosition;

uniform mat4x4 uModel;
uniform mat4x4 uViewproj;
uniform float uScaleFactor;
uniform vec4 uColor = vec4(vec3(0.25f), 0.95f);

out VDataBlock {
  vec4 color;
} OUT;

// ----------------------------------------------------------------------------

void main() {
  vec3 pos = mat3(uModel) * vec3(inPosition.x, 0.0f, inPosition.y);

  vec3 col = vec3(lessThan(abs(pos), vec3(0.1)));

  bool main_axis = any(equal(inPosition.xy, vec2(0.0f)));
  vec3 axis_color = 0.75f*(vec3(1.0) - col);//(vec3(1) - step(abs(pos), vec3(0))) * 0.75f;

  OUT.color.rgb = (main_axis) ? axis_color : uColor.rgb;
  OUT.color.a = uColor.a;

  gl_Position = uViewproj * vec4(uScaleFactor * pos, 1.0f);
}

// ----------------------------------------------------------------------------
