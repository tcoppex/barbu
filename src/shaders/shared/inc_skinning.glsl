#ifndef SHADERS_SHARED_INC_SKINNING_GLSL_
#define SHADERS_SHARED_INC_SKINNING_GLSL_

#include "shared/inc_constants.glsl"

// ----------------------------------------------------------------------------

layout(binding = 0) uniform samplerBuffer uSkinningDatas;

// layout(std430, binding = SSBO_SKINNING_DATA_READ)
// readonly buffer SkinningBuffer {
//   vec4 read_skinning[];
// };

subroutine void skinning_subroutine(in uvec4 _indices, in vec4 _weights, inout vec3 v, inout vec3 n);
subroutine uniform skinning_subroutine uSkinning;

const int kNoJoint = 0xff;

// ----------------------------------------------------------------------------

void apply_skinning(in uvec4 _indices, in vec4 _weights, inout vec3 position, inout vec3 normal) {
  if (_weights.x <= Epsilon()) {
    return;
  }

  // (we can send only three weights and derive the 4th from them)
  _weights.w = 1.0f - (_weights.x + _weights.y + _weights.z);

  uSkinning(_indices, _weights, position, normal);
}

///----------------------------------------------------------------------------
/// SKINNING : DUAL QUATERNION BLENDING
///----------------------------------------------------------------------------

void get_dual_quaternions_matrices(in uvec4 _indices, out mat4 Ma, out mat4 Mb) {
  const ivec4 indices = 2 * ivec4(_indices);

  /// Retrieve the real (Ma) and dual (Mb) part of the dual-quaternions.
  Ma[0] = texelFetch(uSkinningDatas, indices.x+0);
  Mb[0] = texelFetch(uSkinningDatas, indices.x+1);

  Ma[1] = texelFetch(uSkinningDatas, indices.y+0);
  Mb[1] = texelFetch(uSkinningDatas, indices.y+1);

  Ma[2] = texelFetch(uSkinningDatas, indices.z+0);
  Mb[2] = texelFetch(uSkinningDatas, indices.z+1);

  Ma[3] = texelFetch(uSkinningDatas, indices.w+0);
  Mb[3] = texelFetch(uSkinningDatas, indices.w+1);
}

subroutine(skinning_subroutine)
void skinning_DQBS(in uvec4 _indices, in vec4 _weights, inout vec3 v, inout vec3 n) {
  /// Paper :
  ///   "Geometric Skinning with Approximate Dual Quaternion Blending"
  ///   - Kavan et al 2008

  // Retrieve the dual quaternions.
  mat4 Ma, Mb;
  get_dual_quaternions_matrices(_indices, Ma, Mb);

  // Handles antipodality by sticking joints in the same neighbourhood.
  _weights.xyz *= sign(Ma[3] * mat3x4(Ma));

  // Apply weights.
  vec4 A = Ma * _weights;  // real part
  vec4 B = Mb * _weights;  // dual part

  // Normalize.
  const float invNormA = inversesqrt(dot(A, A));
  A *= invNormA;
  B *= invNormA;

  // Transform position.
  v += 2.0f * cross(A.xyz, cross(A.xyz, v) + A.w*v);              // rotate
  v += 2.0f * (A.w * B.xyz - B.w * A.xyz + cross(A.xyz, B.xyz));  // translate

  // Transform normal.
  n += 2.0f * cross(A.xyz, cross(A.xyz, n) + A.w*n);
}


///----------------------------------------------------------------------------
/// SKINNING : LINEAR BLENDING
///----------------------------------------------------------------------------

void get_skinning_matrix(in uint _jointId, out mat3x4 skMatrix) {
  if (_jointId == kNoJoint) {
    skMatrix = mat3x4(1.0f);
    return;
  }

  const int matrixId = 3 * int(_jointId);
  skMatrix[0] = texelFetch(uSkinningDatas, matrixId + 0);
  skMatrix[1] = texelFetch(uSkinningDatas, matrixId + 1);
  skMatrix[2] = texelFetch(uSkinningDatas, matrixId + 2);
}

void get_skinning_matrices(in uvec4 _indices, out mat3x4 matrices[4]) {
  get_skinning_matrix(_indices.x, matrices[0]);
  get_skinning_matrix(_indices.y, matrices[1]);
  get_skinning_matrix(_indices.z, matrices[2]);
  get_skinning_matrix(_indices.w, matrices[3]);
}

vec3 apply_skinning_matrices(in vec4 _vertex, in mat3x4 _matrices[4], in vec4 _weights) {
  /// To allow less texture fetching, and thus better memory bandwith, skinning
  /// matrices are setup as 3x4 on the host (ie. transposed and last column removed)
  /// Hence, matrix / vector multiplications are "reversed".
  mat4x3 M;
  M[0] = _vertex * _matrices[0];
  M[1] = _vertex * _matrices[1];
  M[2] = _vertex * _matrices[2];
  M[3] = _vertex * _matrices[3];
  return (M * _weights).xyz;
}

subroutine(skinning_subroutine)
void skinning_LBS(in uvec4 _indices, in vec4 _weights, inout vec3 v, inout vec3 n) {
  // Retrieve skinning matrices.
  mat3x4 jointMatrices[4];
  get_skinning_matrices( _indices, jointMatrices);

  // Transforms.
  v = apply_skinning_matrices( vec4(v, 1.0), jointMatrices, _weights);
  n = apply_skinning_matrices( vec4(n, 0.0), jointMatrices, _weights);
}

// ----------------------------------------------------------------------------

#endif // SHADERS_SHARED_INC_SKINNING_GLSL_
