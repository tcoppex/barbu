#version 430 core

layout(location=0) in ivec2 inPosition;    // Note : set as integer

out block {
  ivec3 coords_case_numTri;
} Out;

// ----------------------------------------------------------------------------

uniform sampler3D       uDensityVolume_nearest;
uniform isamplerBuffer  uCaseToNumTri;
uniform float           uMargin;

const float kTexelSize    = 1.0f / (textureSize(uDensityVolume_nearest, 0).x);
const float kInvWindowDim = 1.0f / (textureSize(uDensityVolume_nearest, 0).x-1);

void main() {
  // in range [0..chunkdim-1]
  const ivec3 coords = ivec3(inPosition.xy, gl_InstanceID);

  int packed_coords = (coords.x & 0x3f) << 0 |
                      (coords.y & 0x3f) << 6 |
                      (coords.z & 0x3f) << 12;

  const vec3  uvw    = kTexelSize * (coords + uMargin + 0.5f);
  const vec2  offset = vec2(kTexelSize, 0.0f);

  vec4 sideA;
  sideA.x = texture(uDensityVolume_nearest, uvw + offset.yyy).r; 
  sideA.y = texture(uDensityVolume_nearest, uvw + offset.yxy).r;
  sideA.z = texture(uDensityVolume_nearest, uvw + offset.xxy).r;
  sideA.w = texture(uDensityVolume_nearest, uvw + offset.xyy).r;

  vec4 sideB;
  sideB.x = texture(uDensityVolume_nearest, uvw + offset.yyx).r;
  sideB.y = texture(uDensityVolume_nearest, uvw + offset.yxx).r;
  sideB.z = texture(uDensityVolume_nearest, uvw + offset.xxx).r;
  sideB.w = texture(uDensityVolume_nearest, uvw + offset.xyx).r;

# define SATURATE(v)  ivec4(step(0.0f, v))
  const ivec4 iA = SATURATE(sideA);
  const ivec4 iB = SATURATE(sideB);
# undef SATURATE

  int cube_case = (iA.x << 0) | (iA.y << 1) | (iA.z << 2) | (iA.w << 3) |
                  (iB.x << 4) | (iB.y << 5) | (iB.z << 6) | (iB.w << 7);

  Out.coords_case_numTri.x = packed_coords;
  Out.coords_case_numTri.y = cube_case;
  Out.coords_case_numTri.z = texelFetch(uCaseToNumTri, cube_case).r;
}

//------------------------------------------------------------------------------
