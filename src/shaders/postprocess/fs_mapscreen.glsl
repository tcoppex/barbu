#version 430 core

// ---------------------------------------------------------------------------

in layout(location = 0) vec2 vTexCoord;
out layout(location = 0) vec4 fragColor;

// ---------------------------------------------------------------------------

uniform layout(location = 0) sampler2D uTex;

// ---------------------------------------------------------------------------

void main() {
  vec4 rgba = texture(uTex, vTexCoord);
  fragColor = rgba;
}

// ---------------------------------------------------------------------------