#version 430 core

// ----------------------------------------------------------------------------

// Inputs.
layout(lines, invocations = 1) in;
layout(location = 0) in vec3 inPosition[];

// Outputs.
layout(triangle_strip, max_vertices = 4) out;
//layout(location = 0) invariant out float outColorCoeff;

// Uniforms.
uniform mat4 uMVP;
uniform mat4 uView;
uniform float uLineWidth = 0.1;

// ----------------------------------------------------------------------------

// wip
// [This GS is to be used if we want to skip the stream pass and directly
// render the hair ribbons].

void main() {
  outColorCoeff = inColorCoeff[0];

  // View space strand direction.
  vec3 u = vec3(mat3(uView) * (inPosition[1] - inPosition[0]));
  
  // screen-space direction
  u.z = 0.0;
  u = normalize(u);
  vec3 v = vec3( -u.y, u.x, 0.0);

  // Change of basis matrix.
  const vec3 a = normalize(u * mat3(uView));
  const vec3 b = normalize(v * mat3(uView));
  const vec3 c = normalize(cross(a, b));
  const mat3 basis = mat3(a, b, c);

  // Offset vectors.
  const vec3 off = basis * vec3(0,  uLineWidth, 0);

  gl_Position = uMVP * vec4(inPosition[0].xyz + off, 1.0f); EmitVertex();
  gl_Position = uMVP * vec4(inPosition[0].xyz - off, 1.0f); EmitVertex();
  gl_Position = uMVP * vec4(inPosition[1].xyz + off, 1.0f); EmitVertex();
  gl_Position = uMVP * vec4(inPosition[1].xyz - off, 1.0f); EmitVertex();
  EndPrimitive();
}

// ----------------------------------------------------------------------------
