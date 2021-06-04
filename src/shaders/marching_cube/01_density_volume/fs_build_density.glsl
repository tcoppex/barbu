#version 430 core

#include "marching_cube/01_density_volume/inc_density.glsl"

// ----------------------------------------------------------------------------

in block {
  vec3 coordWS;
} In;

out float fragDensity;

// ----------------------------------------------------------------------------

void main() {
  fragDensity = compute_density(In.coordWS);
}

// ----------------------------------------------------------------------------