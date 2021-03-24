#version 430 core

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexcoord;
layout(location = 2) in vec3 inNormal;
//layout(location = 3) in vec3 inBinormal;

// Outputs.
layout(location = 0) out vec3 outPositionWS;
layout(location = 1) out vec2 outTexcoord;
layout(location = 2) out vec3 outNormalWS;
layout(location = 3) out vec3 outViewDirWS;
layout(location = 4) out vec3 outIrradiance;

// Uniforms [todo : transform into an uniform block].
uniform mat4 uMVP;
uniform mat4 uModelMatrix = mat4(1.0);
uniform mat4 uIrradianceMatrices[3];
uniform vec3 uEyePosWS; //

// ----------------------------------------------------------------------------

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
  vec4 pos = vec4(inPosition, 1.0);
  gl_Position = uMVP * pos;

  outPositionWS = vec3(uModelMatrix * pos);
  outTexcoord   = inTexcoord;
  outNormalWS   = normalize(mat3(uModelMatrix) * inNormal);
  outViewDirWS  = normalize(outPositionWS - uEyePosWS);
  outIrradiance = computeIrradiance( outNormalWS, uIrradianceMatrices);
}

// ----------------------------------------------------------------------------