#version 430 core

#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------

// Inputs.
layout(lines, invocations = 1) in;
layout(location = 0) in vec3 inPosition[];
layout(location = 1) in float inCoeff[];

// Outputs.
layout(triangle_strip, max_vertices = 4) out;
layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;

// Uniforms.
uniform mat4 uMVP;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uLineWidth;

// ----------------------------------------------------------------------------

mat3 basisVS(in mat4 viewMatrix, in vec3 p0, in vec3 p1) {
  const mat3 view3 = mat3(viewMatrix);

  // View space direction.
#if 0
  vec3 u = vec3(viewMatrix * vec4(p1 - p0, 1));
  vec3 v = cross(u, vec3(0,1,0));
#else
  // "incorrect" but might give more interesting results.

  vec3 u = view3 * (p1 - p0);
  u.z = 0.0;
  u = normalize(u);
  vec3 v = vec3( -u.y, u.x, 0.0);
#endif

  // Change of basis matrix.
  const vec3 a = normalize(u*view3);
  const vec3 b = normalize(v*view3);
  const vec3 c = normalize(-cross(a, b));
  return mat3(a, b, c);
}

// ----------------------------------------------------------------------------

void fetchVert(in mat4 mvp, in vec4 v, in vec3 tangent) {
  gl_Position = mvp * v;
  outPosition = v.xyz;
  outTangent  = tangent; //
  EmitVertex();
}

void EmitQuadVS(in vec3 A, in vec3 B, in vec3 Y0, in vec3 Y1) {
  const vec3 P0 = A + Y0;
  const vec3 P1 = A - Y0;
  const vec3 P2 = B + Y1;
  const vec3 P3 = B - Y1;

  const vec3 tangent  = normalize(P2 - P0);
  const vec3 binormal = normalize(P1 - P0);
  const vec3 normal   = normalize(cross(tangent, binormal));

  outNormal = normal;
  fetchVert( uMVP, vec4(P0, 1.0f), tangent);
  fetchVert( uMVP, vec4(P1, 1.0f), tangent);

  outNormal = normal; // [! should be different for interpolation]
  fetchVert( uMVP, vec4(P2, 1.0f), tangent);
  fetchVert( uMVP, vec4(P3, 1.0f), tangent);

  EndPrimitive();
}

// ----------------------------------------------------------------------------

void main() {
  // Calculate the side vectors.
  const float width = 0.25 * uLineWidth;
  const vec3 side = 0.5 * width * vec3(
    1.0 - inCoeff[0], 
    1.0 - inCoeff[1], 
    0
  );

  // Calculate the ViewSpace Right and Bottom vectors.
  const mat3 Ybasis = basisVS( uView, inPosition[0], inPosition[1]);
  const vec3 Y0 = Ybasis * side.zxz;
  const vec3 Y1 = Ybasis * side.zyz; 

  // Determine the tangent from the right vector.
  //outTangent = normalize(Y0);

  // Emit the quad.
  EmitQuadVS( inPosition[0].xyz, inPosition[1].xyz, Y0, Y1);
}

// ----------------------------------------------------------------------------

#if 0
void InterpolateGS(line HairVertex2 vertex[2], inout TriangleStream<HairPoint> stream)
{
    if (vertex[0].ID < 0 || vertex[0].ID != vertex[1].ID)
        return;

    float width = g_StrandSizes.Load(vertex[0].ID & g_NumInterpolatedAttributesMinusOne) * g_widthMulGS;

    HairPoint hairPoint;
    float3 tangent = (vertex[1].Position - vertex[0].Position);
    tangent = normalize(tangent);
    float3 worldPos =  vertex[0].Position; // note: this should be the average of the two vertices, and then the other positions also need to change.
    
    float3 eyeVec = TransformedEyePosition - worldPos;
    float3 sideVec = normalize(cross(eyeVec, tangent));
    
    float4x3 pos;
    float3 width0 = sideVec * 0.5 * width * vertex[0].width;
    float3 width1 = sideVec * 0.5 * width * vertex[1].width;
    
    pos[0] = vertex[0].Position - width0;
    pos[1] = vertex[0].Position + width0;
    pos[2] = vertex[1].Position - width1;
    pos[3] = vertex[1].Position + width1;
    
#ifdef INTERPOLATION_LOD_GS
    float2x4 hpos;
    for (int i = 0; i < 2; i++) {
        hpos[i] = mul( float4(pos[i],1.0),ViewProjection);
        hpos[i].xy /= hpos[i].w;
        hpos[i].xy = hpos[i].xy * 0.5 + 0.5; // uv
        hpos[i].xy *= float2(g_ScreenWidth, g_ScreenHeight);
    }

    float w1 = length(hpos[1].xy - hpos[0].xy);
    float p = w1 / g_InterpolationLOD;
    if (w1 < g_InterpolationLOD) {
        float rand = (float)(vertex[0].ID & g_NumInterpolatedAttributesMinusOne) / (float)g_NumInterpolatedAttributes;
        if (rand >= p) return;

        float scale = 1.0/p;
        width0 *= scale;
        width1 *= scale;
        
        pos[0] = vertex[0].Position - width0;
        pos[1] = vertex[0].Position + width0;
        pos[2] = vertex[1].Position - width1;
        pos[3] = vertex[1].Position + width1;        
    }
#endif
    
    float4x2 tex;
    tex[0] = float2(0,vertex[0].tex);
    tex[1] = float2(1,vertex[0].tex);
    tex[2] = float2(0,vertex[1].tex);
    tex[3] = float2(1,vertex[1].tex);       
    
    float2x3 tangents;
    tangents[0] = normalize(vertex[0].Tangent.xyz);
    tangents[1] = normalize(vertex[1].Tangent.xyz);
    hairPoint.scalpTexcoords = vertex[0].scalpTexcoords;
    hairPoint.ID = vertex[0].ID; 



    hairPoint.Position = mul( float4(pos[0],1.0),ViewProjection);
    hairPoint.tex = tex[0];
    hairPoint.shadow = vertex[0].shadow;
    hairPoint.Tangent = tangents[0];
#ifndef SHADOWS_VS
    hairPoint.wPos = pos[0];
#endif
    stream.Append(hairPoint);

    hairPoint.Position = mul( float4(pos[1],1.0),ViewProjection);
    hairPoint.tex = tex[1];
    hairPoint.shadow = vertex[0].shadow;
#ifndef SHADOWS_VS
    hairPoint.wPos = pos[1];
#endif
    stream.Append(hairPoint);

    hairPoint.Position = mul( float4(pos[2],1.0),ViewProjection);
    hairPoint.tex = tex[2];
    hairPoint.shadow = vertex[1].shadow;
    hairPoint.Tangent = tangents[1];
#ifndef SHADOWS_VS
    hairPoint.wPos = pos[2];
#endif
    stream.Append(hairPoint);

    hairPoint.Position = mul( float4(pos[3],1.0),ViewProjection);
    hairPoint.tex = tex[3];
    hairPoint.shadow = vertex[1].shadow;
#ifndef SHADOWS_VS
    hairPoint.wPos = pos[3];
#endif
    stream.Append(hairPoint);

    stream.RestartStrip();    
}
#endif