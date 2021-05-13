#include "animation/skeleton.h"
#include "core/logger.h"

// -----------------------------------------------------------------------------

void Skeleton::add_joint(std::string_view name, int32_t parent_id, glm::mat4 const& inverse_bind_matrix) {
  assert(names.size() < names.capacity());

  auto const joint_name{ name.data() };
  
  names.push_back(joint_name);
  parents.push_back(parent_id);
  inverse_bind_matrices.push_back(inverse_bind_matrix);
  index_map[joint_name] = static_cast<int32_t>(names.size() - 1);
}

void Skeleton::transform_inverse_bind_matrices(glm::mat4 const& inv_world) {
  for (auto &inverse_bind_matrix : inverse_bind_matrices) {
    inverse_bind_matrix *= inv_world;
  }
}

void Skeleton::calculate_globals_inverse_bind_from_locals(JointBuffer_t<glm::mat4> const& inv_locals, glm::mat4 const& inv_world) {
  auto &inv_globals = inverse_bind_matrices;
  assert( inv_locals.size() == inv_globals.size() );
  
  inv_globals[0] = inv_locals[0];
  for (int i = 1; i < njoints(); ++i) {
    inv_globals[i] = inv_globals[parents[i]] * inv_locals[i];
  }
  transform_inverse_bind_matrices(inv_world);
}

void Skeleton::calculate_global_bind_matrices() {
  if (!global_bind_matrices.empty()) {
    LOG_WARNING( "Skeleton global bind matrices already computed." );
    return;
  }

  global_bind_matrices.resize(njoints());
  for (int i = 0; i < njoints(); ++i) {
    global_bind_matrices[i] = glm::inverse( inverse_bind_matrices[i] );
  }
}

// -----------------------------------------------------------------------------
