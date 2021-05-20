#version 430 core

#include "hair/interop.h"

// ----------------------------------------------------------------------------

// READ
layout(std430, binding = SSBO_HAIR_SIM_POSITION_READ)
readonly buffer PositionBufferA {
  vec4 read_positions[];
};
layout(std430, binding = SSBO_HAIR_SIM_VELOCITY_READ)
readonly buffer VelocityBufferA {
  vec4 read_velocities[];
};
// layout(std430, binding = SSBO_HAIR_SIM_TANGENT_READ)
// readonly buffer TangentBufferA {
//   vec4 read_tangents[];
// };

// WRITE
layout(std430, binding = SSBO_HAIR_SIM_POSITION_WRITE)
writeonly buffer PositionBufferB {
  vec4 write_positions[];
};
layout(std430, binding = SSBO_HAIR_SIM_VELOCITY_WRITE)
writeonly buffer VelocityBufferB {
  vec4 write_velocities[];
};
// layout(std430, binding = SSBO_HAIR_SIM_TANGENT_WRITE)
// writeonly buffer TangentBufferB {
//   vec4 write_tangents[];
// };

// ----------------------------------------------------------------------------

uniform mat4 uModel = mat4(1.0f);

uniform float uTimeStep;
uniform float uMaxVelocity = 100.0f;
uniform float uScaleFactor = 1.0f;    // (just for fun)

uniform vec4 uBoundingSphere = vec4( 0.0, 0.0, 0.0, 1.0);

// ----------------------------------------------------------------------------

struct Particle_t {
  vec3 position;
  vec3 velocity;
  float restlength;
};

Particle_t UnpackParticle(uint index) {
  Particle_t p; vec4 data;
  data = read_positions[index];
  p.position    = data.xyz;
  p.restlength  = data.w;  
  data = read_velocities[index];
  p.velocity    = data.xyz;
  return p;
}

void PackParticle(uint index, in Particle_t p) {
  write_positions[index]  = vec4(p.position, p.restlength);
  write_velocities[index] = vec4(p.velocity, 0);
}

// ----------------------------------------------------------------------------

vec3 CalculateForces(in Particle_t p) {
  const vec3 gravity = vec3(0.0f, -9.81f, 0.0f);
  const float kForceCoeff = 20.0;

  vec3 force = kForceCoeff * gravity;

  return force;
}

void VelocityControl( inout vec3 velocity, float maxIntensity) {
  const float dp = dot(velocity, velocity);
  if (dp > maxIntensity * maxIntensity) {
    velocity *= maxIntensity * inversesqrt(dp);
  }
}

// ----------------------------------------------------------------------------

// Particles data shared with the strands (thread group).
shared Particle_t s_particles[HAIR_MAX_PARTICLE_PER_STRAND];

// ----------------------------------------------------------------------------

void DistanceConstraint(uint index, inout Particle_t p) {
  // Solve the strand dynamics using the DFTL algorithm.
  // ref. "Fast Simulation of Inextensible Hair and Fur" - M. Müller & al. [2012]
  // ----
  
  // @note the paper suggest a general damp scale closer to one.
  // This one is highly dependent on the underlying number of strands (here '5').
  float di = index / float(HAIR_MAX_PARTICLE_PER_STRAND-1);
        di = smoothstep(0.0, 1.0, di)*di;
  const float dftl_damp_scale = mix( 0.80, 0.94, di); 

  // Share the particle with its local group (ie. rest of the strands).
  s_particles[index] = p; 
  memoryBarrierShared();

  // Only the first thread run the iterative pass.
  if (0 == index) {  
    for (int i=1; i < HAIR_MAX_PARTICLE_PER_STRAND; ++i) {
      const vec3 p0 = s_particles[i-1].position;
      const vec3 p1 = s_particles[i].position;
      const vec3 vdiff = p1 - p0;
      const vec3 p1_bis = p0 + (uScaleFactor * s_particles[i].restlength) * normalize(vdiff);
      s_particles[i].position = p1_bis;
      s_particles[i].velocity = (p1_bis - p1); 
    }

    for (int i=1; i < HAIR_MAX_PARTICLE_PER_STRAND-1; ++i) {
      s_particles[i].velocity = s_particles[i+1].velocity * dftl_damp_scale;
    }
  }
  memoryBarrierShared();

  // Update the local copy of the particle.
  p = s_particles[index];
}

void CollideSphere(float dp_sign, in vec3 center, float radius, inout Particle_t p) {
  const vec3 pt = p.position - center;
  const float dp = dot(pt, pt);

  if ((dp_sign * dp) < (radius * radius)) {
    const vec3 n = dp_sign * pt * inversesqrt(dp);

    p.position = center + radius * n;
    p.velocity = reflect(p.velocity, n);
  }
}

void CollideSphereExternal(in vec3 center, float radius, inout Particle_t p) {
  CollideSphere(+1.0, center, radius, p);
}

void CollideSphereInternal(in vec3 center, float radius, inout Particle_t p) {
  CollideSphere(-1.0, center, radius, p);
}

void CollisionConstraint(uint index, inout Particle_t p) {
  if (index > 0) {
    CollideSphereExternal( uBoundingSphere.xyz, uBoundingSphere.w, p);
  }
}

void SatisfyConstraints(int num_constraints_iterations, uint index, inout Particle_t p) {
  for (int i=0; i < num_constraints_iterations; ++i) {
    DistanceConstraint(index, p);
    CollisionConstraint(index, p);
  }
  groupMemoryBarrier();
}

// ----------------------------------------------------------------------------

// Note : All strands share the same number of 'particles' control points,
//        the algorithm would need to be slightly changed if we need a varying number.
//        (mostly thread dependent stuffs, and an additional buffer of CP count per strands)

layout(local_size_x = HAIR_MAX_PARTICLE_PER_STRAND) in;
void main() {
  const uint gid = gl_GlobalInvocationID.x;
  const vec3 dt = vec3(uTimeStep);

  // Save particle to local memory.
  Particle_t p = UnpackParticle(gid);
  const vec3 lastPosition = p.position;

  // Integrate positions.
  if (gl_LocalInvocationIndex > 0) 
  {
    const vec3 force = CalculateForces(p);
    p.position = fma(dt*dt, force, fma(dt, p.velocity, p.position));
  } 
  else 
  {
    // For root vertice, transform them instead.
    // !! should be calculated from their object space positions, eg. kept from another buffer.
    // (ideally post skinning / blendshape pass)

    mat4 model = mat4(1.0); // uModel;
    p.position = vec3(model * vec4(lastPosition, 1.0));
    p.velocity = p.position - lastPosition;
  }
  groupMemoryBarrier(); //

  // Satisfy constraints.
  const int kNumContraintsIteration = 8; //
  SatisfyConstraints( kNumContraintsIteration, gl_LocalInvocationIndex, p);

  // Update velocity (cf DFTL paper, Eq. 9).
  // p.velocity = ((p.position - lastPosition) - p.velocity) / dt;

  // // Velocity control.
  // VelocityControl(p.velocity, uMaxVelocity);

  // Save back particles to global memory.
  PackParticle(gid, p);
}
