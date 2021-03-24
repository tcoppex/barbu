#version 430 core

// ----------------------------------------------------------------------------

out layout(location = 0) vec2 vTexCoord;

// ----------------------------------------------------------------------------

//
// This shader use a trick to render a NDC-mapped quad using a generated
// triangle with texcoords mapping the screen. 
//
// On the device simply create an empty VertexArray (ie. without attribute arrays) 
// and call glDrawArrays(GL_TRIANGLES, 0, 3).
//
// This is the OpenGL-compatible version.
//
void main() {
  vTexCoord.s = (gl_VertexID << 1) & 2;
  vTexCoord.t = gl_VertexID & 2;
  gl_Position = vec4(2.0f*vTexCoord - 1.0f, 0.0f, 1.0f);
}

// ----------------------------------------------------------------------------