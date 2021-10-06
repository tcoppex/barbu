#include "fx/animation/skeleton_controller.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

#include "core/logger.h"
#include "utils/mathutils.h"

#ifdef BARBU_NPROC_MAX
#define LOOP_NTHREADS  (BARBU_NPROC_MAX)
#else
#define LOOP_NTHREADS  4
#endif

// -----------------------------------------------------------------------------

namespace {

/// Apply a linear interpolation on two samples.
void LerpSamples(const AnimationSample_t &s1,
                 const AnimationSample_t &s2,
                 float const factor,
                 AnimationSample_t &dst_sample)
{
  int32_t const njoints_ = static_cast<int32_t>(s1.joints.size());
  dst_sample.joints.resize(njoints_);

  #pragma omp parallel for schedule(static) num_threads(LOOP_NTHREADS)
  for (int32_t i = 0; i < njoints_; ++i) {
    auto const& J1{ s1.joints[i] };
    auto const& J2{ s2.joints[i] };
    auto &dst{ dst_sample.joints[i] };

    // (for quaternions use shortMix (slerp) or fastMix (nlerp) but *NOT* mix).
    dst.qRotation    = glm::shortMix( J1.qRotation,       J2.qRotation,     factor);
    dst.vTranslation =      glm::mix( J1.vTranslation,    J2.vTranslation,  factor);
    dst.fScale       =      glm::mix( J1.fScale,          J2.fScale,        factor);

    // LOG_MESSAGE( __FUNCTION__, dst.fScale, J1.fScale, J2.fScale, factor);
    // LOG_MESSAGE( __FUNCTION__, dst.vTranslation.x, J1.vTranslation.x, J2.vTranslation.x, factor);
  }
}

/// Generate a sample for sequence_clip at global_time.
bool ComputePose(float const global_time,
                 SequenceClip_t& sequence_clip,
                 AnimationSample_t& dst_sample)
{
  // [todo: handle compressed joints data]
  
  float local_time(0.0f);

  if (sequence_clip.evaluate_localtime(global_time, local_time)) {
    sequence_clip.bEnable = false;
    return false;
  }
  int32_t const next_frame = +1; //

  auto const* clip = reinterpret_cast<AnimationClip_t const*>(sequence_clip.action_ptr); //

  // Interpolated frame.
  float const lerped_frame = local_time * clip->framerate;

  // Found the frame boundaries.
  int32_t const frame_a = static_cast<int32_t>(lerped_frame) % clip->framecount;
  int32_t const frame_b = (frame_a + next_frame) % clip->framecount;

  // Compute the correct time sample for the pose.
  auto const& s1 = clip->samples[frame_a];
  auto const& s2 = clip->samples[frame_b];
  float const lerp_factor = lerped_frame - static_cast<float>(frame_a); // glm::fract(lerped_frame)

  LerpSamples(s1, s2, lerp_factor, dst_sample);

  // LOG_INFO( "{", lerped_frame, "} =", local_time, "*", clip->framerate);
  // LOG_INFO( "[", frame_a, frame_b, "]", "{", lerped_frame, "}");

  return true;
}

}  // namespace

// -----------------------------------------------------------------------------

//
//  Shared structure to handle sampling a clip from a skinning animation.
//
struct SkinningFrame_t {
  AnimationSampleBuffer_t samples;
  int32_t num_active_clips;

  /* Compute the static pose derived from each contributing clips. */
  void compute_poses(float const global_time, Sequence_t &sequence) {
    num_active_clips = 0;
    samples.resize(sequence.size());

    for (auto &sc : sequence) {
      if (sc.bEnable && ComputePose(global_time, sc, samples[num_active_clips])) {
        ++num_active_clips;
      }
    }
  }
};

static SkinningFrame_t sFrame;

// -----------------------------------------------------------------------------

bool SkeletonController::evaluate(SkinningMode const mode, SkeletonHandle skeleton, float const global_time, Sequence_t &sequence) {
  if (nullptr == skeleton) {
    return false;
  }

  // 1) Compute the static sampling pose from each contributing clips.
  sFrame.compute_poses( global_time, sequence);

  if (sFrame.num_active_clips <= 0) {
    LOG_DEBUG_INFO( "No animation clips were provided." );
    return false;
  }

  // Resize buffer data when needed.
  njoints_ = skeleton->njoints();
  if (local_pose_.joints.size() < static_cast<size_t>(njoints_)) {
    local_pose_.joints.resize( njoints_ );
    global_pose_matrices_.resize( njoints_ );
    skinning_matrices_.resize( njoints_ );
    if (SkinningMode::DualQuaternion == mode) {
      dual_quaternions_.resize( njoints_ );
    }
  }

  // 2) Blend between poses to get a unique local pose.
  blend_poses(sequence);

  // (we could post-process the blend pose before generating the skinning data).

  // 3) Generate the global pose matrices.
  generate_global_pose_matrices(skeleton);

  // 4) Generate the final skinning datas.
  generate_skinning_datas(mode, skeleton);

  return true;
}


void SkeletonController::blend_poses(Sequence_t const& sequence) {
  auto const& samples = sFrame.samples;

  // Bypass the weighting if there is only one active clip.
  if (1 == sFrame.num_active_clips) {
    auto const joints_start{ samples[0].joints.cbegin() };
    std::copy(joints_start, joints_start + njoints_, local_pose_.joints.begin());
    LOG_DEBUG_INFO( __FUNCTION__, ": single clip copy." );
    return;
  }

  //
  // Compute local poses by blending each contributing samples by the factor
  // previously calculated by the blend tree. 
  // Suppose blending associativity (ie. flat weighted average).
  //

  // 1) Calculate the total weight for normalization.
  float sum_weights = 0.0f;
  
  // [ we might switch to omp by removing the iterator ]
  //# pragma omp parallel for reduction(+:sum_weights) schedule(static) num_threads(LOOP_NTHREADS)
  for (auto const &sc : sequence) {
    sum_weights += (sc.bEnable) ? sc.weight : 0.0f;
  }
  sum_weights = (sum_weights == 0.0f) ? 1.0f : sum_weights; //

  // 2) Copy the first weighted action as base.
  auto it = sequence.cbegin();
  {
    float const w = it->weight / sum_weights;
    
    #pragma omp parallel for schedule(static) num_threads(LOOP_NTHREADS)
    for (int32_t i = 0; i < njoints_; ++i) {
      auto const &src = samples[0].joints[i];
            auto &dst = local_pose_.joints[i];
      
      dst.qRotation    = w * src.qRotation;
      dst.vTranslation = w * src.vTranslation;
      dst.fScale       = w * src.fScale;
    }
  }

  // 3) Blend the rest.
  int32_t sid = 1;
  for (it = ++it; sid < sFrame.num_active_clips; ++it, ++sid) {
    auto const& sample = samples[sid];

    // Cope with antipodality by checking the quaternion neighborhood.
    float const sign_q{ glm::sign(glm::dot(
      sample.joints[0].qRotation, 
      sample.joints[njoints_-1].qRotation
    ))};

    float const w   = it->weight / sum_weights;
    float const w_q = w * sign_q;

    // [could swap the 2 loops and do 3 reduces operations instead]
    #pragma omp parallel for schedule(static) num_threads(LOOP_NTHREADS)
    for (int32_t i = 0; i < njoints_; ++i) {
      auto const &src = sample.joints[i];
            auto &dst = local_pose_.joints[i];

      dst.qRotation    += w_q * src.qRotation;
      dst.vTranslation += w   * src.vTranslation;
      dst.fScale       += w   * src.fScale;
    }

    // Normalize quaternion lerping.
    for (auto &joint : local_pose_.joints) {
      joint.qRotation /= glm::length(joint.qRotation);
    }
  }
}

void SkeletonController::generate_global_pose_matrices(SkeletonHandle skeleton) {
  // [ scaling is not applied ]

  // Compute local matrices.
  #pragma omp parallel for schedule(static) num_threads(LOOP_NTHREADS)
  for (int32_t i = 0; i < njoints_; ++i) {
    auto const &joint = local_pose_.joints[i];
    global_pose_matrices_[i]  = glm::translate(glm::mat4(1.0f), joint.vTranslation)
                              * glm::mat4_cast(joint.qRotation)
                              ;
  }

  // The root should use the world matrix.
  global_pose_matrices_[0] = skeleton->global_bind_matrices[0]; // xxx
  // global_pose_matrices_[0] = world_matrix * global_pose_matrices_[0];

  // Multiply non-root bones with their parent.
  for (int32_t i = 1; i < njoints_; ++i) {
    auto const parent_id = skeleton->parents[i];
    global_pose_matrices_[i] = global_pose_matrices_[parent_id] 
                             * global_pose_matrices_[i]
                             ;
  }
}

void SkeletonController::generate_skinning_datas(SkinningMode const mode, SkeletonHandle skeleton) {
  // Generate skinning matrices, transposed to fill a 3x4 matrix.
  #pragma omp parallel for schedule(static) num_threads(LOOP_NTHREADS)
  for (int32_t i = 0; i < njoints_; ++i) {
    glm::mat4 const skin_matrix{
      global_pose_matrices_[i] * skeleton->inverse_bind_matrices[i]
    };
    skinning_matrices_[i] = glm::mat3x4(glm::transpose(skin_matrix));
  }

  // Convert Skinning Matrices to Dual Quaternions.
  if (SkinningMode::DualQuaternion == mode) {
    #pragma omp parallel for schedule(static) num_threads(LOOP_NTHREADS)
    for (int32_t i = 0; i < njoints_; ++i) {
      dual_quaternions_[i] = glm::dualquat(skinning_matrices_[i]);
    }
  }
}

// -----------------------------------------------------------------------------
