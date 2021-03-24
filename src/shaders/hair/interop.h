#ifndef SHADERS_HAIR_INTEROP_H_
#define SHADERS_HAIR_INTEROP_H_

// ----------------------------------------------------------------------------

// Determine the number of control points per strands and the size
// of the simulation kernel.
#define HAIR_MAX_PARTICLE_PER_STRAND          3

// ----------------------------------------------------------------------------
// SIMULATION

// Hair simulation attributes layout :
// 1 : XYZ Position + W restlength
// 2 : XYZ Velocity + W unused
// 3 : XYZ Tangent  + W unused

// Attributes 'read'.
#define SSBO_HAIR_SIM_POSITION_READ           0
#define SSBO_HAIR_SIM_VELOCITY_READ           1
#define SSBO_HAIR_SIM_TANGENT_READ            2

// Attributes 'write'.
#define SSBO_HAIR_SIM_POSITION_WRITE          3
#define SSBO_HAIR_SIM_VELOCITY_WRITE          4
#define SSBO_HAIR_SIM_TANGENT_WRITE           5

// Helpers.
#define SSBO_HAIR_SIM_FIRST_BINDING           SSBO_HAIR_SIM_POSITION_READ
#define NUM_SSBO_HAIR_SIM_ATTRIBS             (SSBO_HAIR_SIM_POSITION_WRITE - SSBO_HAIR_SIM_FIRST_BINDING)

// ----------------------------------------------------------------------------
// TESSELLATION / TRANSFORM FEEDBACK

#define SSBO_HAIR_TF_RANDOMBUFFER             0
#define HAIR_TF_RANDOMBUFFER_SIZE             4096

#define BINDING_HAIR_TF_ATTRIB_OUT            0

// ----------------------------------------------------------------------------
// RENDERING

#define BINDING_HAIR_RENDER_ATTRIB_IN         0

// ----------------------------------------------------------------------------

#endif // SHADERS_HAIR_INTEROP_H_
