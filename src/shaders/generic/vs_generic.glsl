#version 430 core

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexcoord;
layout(location = 2) in vec3 inNormal;

// Outputs.
layout(location = 0) out vec3 outPositionWS;
layout(location = 1) out vec2 outTexcoord;
layout(location = 2) out vec3 outNormalWS;
layout(location = 3) out vec3 outIrradiance;

// Uniforms [ use uniform block ].
uniform mat4 uMVP;
uniform mat4 uModelMatrix;
uniform mat4 uIrradianceMatrices[3];

// ----------------------------------------------------------------------------

// [ move to the fragment shader ]
vec3 computeIrradiance(in vec3 normalWS, in mat4 irradianceMatrices[3]) {
  vec4 n = vec4( normalWS, 1.0);
  return vec3(
    dot( n, irradianceMatrices[0] * n),
    dot( n, irradianceMatrices[1] * n),
    dot( n, irradianceMatrices[2] * n)
  );
}

// ----------------------------------------------------------------------------

void main() {
  const vec4 pos = vec4(inPosition, 1.0);

  gl_Position   = uMVP * pos;
  outPositionWS = vec3(uModelMatrix * pos);
  outTexcoord   = inTexcoord;
  outNormalWS   = normalize(mat3(uModelMatrix) * inNormal);
  outIrradiance = computeIrradiance( outNormalWS, uIrradianceMatrices);
}

// ----------------------------------------------------------------------------