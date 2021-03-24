#ifndef UTILS_CPU_PARTICLES_H_
#define UTILS_CPU_PARTICLES_H_

#include <cmath>
#include <vector>
#include "glm/glm.hpp"

// ----------------------------------------------------------------------------

//
// Single header CPU particle system with a 2nd degree Verlet integrator.
//
namespace cpu_particle {

// "Structure Of Array" attributes for particles.
struct Buffer_t {
  std::vector<glm::vec3>  p0;      //< last position,
  std::vector<glm::vec3>  p1;      //< current position,
  std::vector<float>      radius;  //< radius used for collision,
  std::vector<bool>       tied;    //< true if the particle is static
                                   //  [alternatively use inverse of mass with ~0 for fixed].

  int size() const {
    return static_cast<int>(p0.size());
  }

  void resize(size_t size) {
    p0.resize(size);
    p1.resize(size);
    radius.resize(size, 0.0f);
    tied.resize(size, false);
  }

  void reset_delayed_positions() {
    p0.assign(p1.begin(), p1.end());
  }
};

// Constraints shared between two particles.
struct Spring_t {
  enum Type {
    STRUCTURAL = 0,
    SHEAR      = 1,
    BEND       = 2,
    kNumSpringType
  } type;

  int pointA;
  int pointB;
  float restLength;
  float Ks;
  float Kd;
};

// Bounding sphere used for simple collision detection.
struct Sphere_t {
  glm::vec3 center;
  float radius;
};

// Set of particles with forces and constraints.
struct System_t {
  struct Params_t {
    float global_damping;
  };

  Params_t                params;               // System's parameters
  Buffer_t                particles;            // Particles
  std::vector<Spring_t>   springs;              // Springs constraints
  std::vector<glm::vec3>  directional_forces;   // Directional forces
  std::vector<Sphere_t>   bounding_spheres;     // Bounding spheres

  int numParticles() const { return particles.size(); }
  int numSprings()   const { return springs.size(); }
  int numForces()    const { return directional_forces.size(); }
};

// ----------------------------------------------------------------------------

class VerletIntegrator {
 public:
  void simulate(float const dt, int const nIteration, System_t& psystem)
  {
    // Resize the forces buffer if needed.
    if (forces_accum_.empty() || psystem.numParticles() < accumBufferSize()) {
      forces_accum_.resize(psystem.numParticles());
    }

    for (int i = 0; i < nIteration; ++i) {
      accumulateForces(dt, psystem);
      accumulateSprings(dt, psystem);
      integrate(dt, psystem.particles);
      satisfyConstraints(dt, psystem);
    }
  }

 private:
  void accumulateForces(float const dt, System_t const& psystem)
  {
    float const dampCoeff = psystem.params.global_damping / dt;

    // Reset the forces buffer.
    forces_accum_.assign(forces_accum_.size(), glm::vec3(0.0f));

    // Accumulate forces.
    Buffer_t const& particles = psystem.particles;
    for (int i = 0; i < particles.size(); ++i) {
      auto &forceAccum = forces_accum_[i];
      for (auto const& force : psystem.directional_forces) {
        forceAccum += force;
      }
      // Add damping (viscosity).
      forceAccum += dampCoeff * (particles.p1[i] - particles.p0[i]); 
    }
  }

  void accumulateSprings(float const dt, System_t const& psystem)
  {
    Buffer_t const& particles = psystem.particles;

    for (auto const& spring : psystem.springs) {      
      auto const& pA0 = particles.p0[spring.pointA];
      auto const& pB0 = particles.p0[spring.pointB];
      auto const& pA1 = particles.p1[spring.pointA];
      auto const& pB1 = particles.p1[spring.pointB];

      // Velocity & positional differences.
      glm::vec3 const dv = ((pA1 - pA0) - (pB1 - pB0)) / dt;
      glm::vec3 dp = pA1 - pB1;

      // Springs factors.
      float const dpLength = glm::length(dp);
      float const shearCoeff = -spring.Ks * (dpLength - spring.restLength);
      float const dampCoeff  = +spring.Kd * glm::dot(dv, dp) / dpLength;

      // Final force.
      if (fabsf(dpLength) > 1.e-4f) {
        dp /= dpLength;
      }
      glm::vec3 const springForce = (shearCoeff + dampCoeff) * dp;

      forces_accum_[spring.pointA] += springForce;
      forces_accum_[spring.pointB] -= springForce;
    }
  }

  void integrate(float const dt, Buffer_t &particles)
  {
    float const dt2 = dt * dt;

    for (int i = 0; i < particles.size(); ++i) {
      auto &p0 = particles.p0[i];
      auto &p1 = particles.p1[i];

      auto const last_p1 = p1;
      p1 = 2.0f * p1 - p0 + dt2 * forces_accum_[i];
      p0 = last_p1;
    }
  }

  void satisfyConstraints(float const dt, System_t &psystem)
  {
    auto const& bspheres = psystem.bounding_spheres;
    auto& particles = psystem.particles;
    
    for (int i = 0; i < particles.size(); ++i) {
      auto& position = particles.p1[i];

      // Bounding sphere collision
      float const particle_radius = particles.radius[i];
      for (const auto &sphere : bspheres)
      {
        auto sphere_to_particle = position - sphere.center;
        float const d2 = glm::dot( sphere_to_particle, sphere_to_particle);
        float const r = sphere.radius + particle_radius;

        if (d2 < r*r) {
          sphere_to_particle = (r / sqrtf(d2)) * sphere_to_particle;
          position = sphere.center + sphere_to_particle;
        }
      }

      // [debug] Simulation bounding box.
      position = glm::clamp(position, glm::vec3(-200.0f), glm::vec3(+200.0f));

      // Fixed particles.
      if (particles.tied[i]) {
        position = particles.p0[i];
      }
    }
  }

  int accumBufferSize() const { return static_cast<int>(forces_accum_.size()); }

  std::vector<glm::vec3> forces_accum_;
};

} // namespace "particle"

// ----------------------------------------------------------------------------

#endif  // UTILS_CPU_PARTICLES_H_
