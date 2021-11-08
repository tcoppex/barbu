#include "ecs/scene_hierarchy.h"

#include <algorithm>
#include <vector>

#include "glm/gtc/matrix_inverse.hpp"
#include "im3d/im3d.h"  // (used for debug render)

#include "core/camera.h"
#include "core/global_clock.h"
#include "ui/views/ecs/SceneHierarchyView.h"

// ----------------------------------------------------------------------------

// Default name given to rig entity loaded from model.
static constexpr char const* kDefaultRigEntityName{ "[rig]" };

// Resolution used to render debug sphere.
constexpr int32_t kDebugSphereResolution = 16;

constexpr bool kEnableLoadRigHierarchy = true;

// ----------------------------------------------------------------------------

SceneHierarchy::~SceneHierarchy() {
  ui_view.reset();
}

void SceneHierarchy::init() {
  ui_view = std::make_shared<views::SceneHierarchyView>(*this);
}

void SceneHierarchy::update(float const dt, Camera const& camera) {
  // Clear per-frame data.
  frame_.clear();
  frame_.globals.resize(entities_.size(), glm::mat4(1.0f));

  // Update the scene entities hierarchically.
  updateHierarchy(dt);

  // Update UI selection size.
  ui_view->selected_.resize(entities_.size(), false); //

  // Retrieve specific entities.
  for (auto& e : entities_) {
    if (isSelected(e)) {
      frame_.selected.push_back( e );
    }

    if (e->has<SphereColliderComponent>()) {
      frame_.colliders.push_back( e );
    }
  }

  // ----------------------------------------

  // Retrieve renderable entities.
  for (auto& e : entities_) {
    if (e->has<VisualComponent>()) {
      frame_.drawables.push_back( e );
    }
  }

  // Sort the drawables front to back.
  sortDrawables(camera);

  // Animate nodes with skinning (for now, suppose them all drawables).
  float const global_time = static_cast<float>(GlobalClock::Get().applicationTime()); //
  for (auto& e : frame_.drawables) {
    if (!e->has<SkinComponent>()) {
      continue;
    }
    
    // Calculate skinning matrices.
    if (auto &skin = e->get<SkinComponent>(); skin.update(global_time)) {
      // Update rig entity global matrix from skinning.
      // [ hence we might want to avoid computing their global uselessly beforehand ]
      auto &visual = e->get<VisualComponent>();
      if (auto rig = visual.rig(); rig) {
        auto const& rig_global = globalMatrix(rig->index());
        auto const& controller = skin.controller();
        auto const& global_pose_matrices = controller.global_pose_matrices();

        // Map skeleton joint index to their rig entity.
        auto &skeleton_map = skin.skeletonMap(); //
        assert(!skeleton_map.empty());

        for (int32_t joint_id = 0; joint_id < controller.njoints(); ++joint_id) {
          auto const& global_pose = global_pose_matrices[joint_id];
          auto const entity_index = skeleton_map[joint_id]->index(); //
          assert( entity_index > -1 );

          frame_.globals[entity_index] = rig_global * global_pose;
        }
      }
    }
  }
}

void SceneHierarchy::removeEntity(EntityHandle entity, bool bRecursively) {
  if (nullptr == entity) {
    return;
  }

  // [todo] Make sure the assets are properly released when not used anymore.
  
  // Remove rig from loaded model. 
  if (entity->has<VisualComponent>()) {
    auto visual = entity->get<VisualComponent>();
    if (auto rig = visual.rig(); rig) {
      removeEntity( rig, true);
      visual.setRig(nullptr);
    }
  } else if(entity->name() == kDefaultRigEntityName) {
    if (auto parent = entity->parent(); parent && parent->has<VisualComponent>()) {
      parent->get<VisualComponent>().setRig(nullptr);
      LOG_DEBUG_INFO( "Auto removed rig from parent." );
    }
  }

  if (entity->parent_) {
    // Remove the entity from its parent.
    auto &siblings = entity->parent_->children_;
    for (auto it = siblings.begin(); it != siblings.end(); ++it) {
      if (*it == entity) {
        siblings.erase(it);
        break;
      } 
    }

    if (!bRecursively) {
      // Add entity children to its parent.
      siblings.insert( siblings.end(), entity->children_.begin(), entity->children_.end());
    }
  }

  // Remove entity from the list of entities.
  entities_.remove( entity );

  if (bRecursively) {
    // Remove children recursively.
    for (auto &child : entity->children_) {
      removeEntity(child, bRecursively);
    }
  } else {
    // Change children's parent.
    for (auto &child : entity->children_) {
      child->parent_ = entity->parent_;
    }
  }

  // Clear entity's relationships.
  entity->parent_ = nullptr;
  entity->children_.clear();
}

void SceneHierarchy::resetEntity(EntityHandle entity, bool bRecursively) {
  if (nullptr == entity) {
    return;
  }

  // Remove children recursively.
  if (bRecursively) {
    for (auto &child : entity->children_) {
      resetEntity(child, bRecursively);
    }
  }

  // Reset transform.
  entity->transform().reset();
}

EntityHandle SceneHierarchy::importModel(std::string_view filename) {
  /// This load the whole geometry of the file as a single mesh.
  /// [ create an importer for scene structure. ]

  // When successful, add a new model entity to the scene.
  if (auto mesh = MESH_ASSETS.create( AssetId(filename) ); mesh && mesh->loaded()) {
    // Import materials separetely.
    MATERIAL_ASSETS.import_from_meshdata( ResourceId(filename) );

    // Retrieve the file basename.
    auto const basename = Logger::TrimFilename( std::string(filename) );    
    
    // Create a new mesh entity node.
#if 0
    auto entity = Entity::Create<ModelEntity>(basename, mesh); //
    addEntity(entity);
#else
    auto entity = createEntity<ModelEntity>( basename, mesh);
#endif

    // Handle skinned model.
    if (entity) {
      if (auto skl = entity->skeleton(); skl) {
        // ----------------------------
        // [ Work In Progress]

        // Add a skinning component.
        auto& skin = entity->add<SkinComponent>();
        skin.setSkeleton( skl );

        // [debug]
        // Set the first animation clips on loop.
        if (auto& clips = skl->clips; !clips.empty()) {
          auto& sequence = skin.sequence();
          auto& first_clip = clips[0];
          first_clip.bLoop = true;
          sequence.push_back( SequenceClip_t(&first_clip) );
        } else {
          LOG_WARNING( "No clips were found for skinned entity", basename );
        }

        // Add a mockup rig as the entity's sub-hierarchy.  
        if constexpr (kEnableLoadRigHierarchy) {
          // Note : 
          // This disrupt the entity focus feature on select all / default mode.

          // Calculate the bones globals binds.
          // ( costly as it inverts the inverse global bind matrices )
          skl->calculate_global_bind_matrices();

          // Create an upper rig entity.
#if 0
          auto rig_root = Entity::Create( kDefaultRigEntityName ); 
          addChildEntity( entity, rig_root);
#else
          auto rig_root = createChildEntity( entity, kDefaultRigEntityName);
#endif

          // Add the rig root to the visual component.
          auto &visual = entity->get<VisualComponent>(); 
          visual.setRig( rig_root );

          // We use a map intern to the skin component to find entities from their
          // joint index.
          auto const njoints{ skl->njoints() };
          auto &skeleton_map = skin.skeletonMap();  //
          skeleton_map.resize( njoints );
          
          // Add joint entities to the hierarchy.
          for (int32_t i = 0; i < njoints; ++i) {
            // Entity's parent.
            auto const parent_id = skl->parents[i];
            auto parent = (parent_id > -1) ? skeleton_map[parent_id] : rig_root;
            
            // Create the new entity in the hierarchy.
            auto joint_entity = createChildEntity( parent, skl->names[i]);

            // Update the entity local matrix using skeleton matrices.
            auto const& parent_inv_matrix = (parent_id > -1) ? skl->inverse_bind_matrices[parent_id] : glm::mat4(1.0f);
            joint_entity->localMatrix() = parent_inv_matrix * skl->global_bind_matrices[i];

            // Add the entity to the skeleton map.
            skeleton_map[i] = joint_entity;
          }
        }
        // ----------------------------
      }
    }
    return entity;
  }

  return nullptr;
}

void SceneHierarchy::select(EntityHandle entity, bool status) {
  if (entity->index() < 0) {
    LOG_ERROR( "Entity index is invalid, it must have been created before scene internal update." );
    return;
  }
  
  // [fixme] using the UI as state holder is not great.
  ui_view->select(entity->index(), status);
}

void SceneHierarchy::toggleSelect(bool status) {
  ui_view->selectAll(status);
}

bool SceneHierarchy::isSelected(EntityHandle entity) const {
  return entity ? ui_view->isSelected(entity->index()) : false;
}

glm::vec3 SceneHierarchy::pivot(bool selected) const {
  auto const& entities = (selected && !frame_.selected.empty()) ? frame_.selected : entities_;

  glm::vec3 pivot{ 0.0f };
  if (!entities.empty()) {
    for (auto const& e : entities) {
      pivot += globalPosition(e);
    }
    pivot /= entities.size();
  }

  return pivot;
}

glm::vec3 SceneHierarchy::centroid(bool selected) const {
  auto const& entities = (selected && !frame_.selected.empty()) ? frame_.selected : entities_;

  glm::vec3 center{ pivot(selected) };
  if (!entities.empty()) {
    for (auto const& e : entities) {
      auto p = glm::vec4(e->centroid(), 1);
      
      // compensate for local scaling.
      auto const& lm = e->localMatrix(); 
      p = glm::scale(glm::mat4(1.0), glm::vec3(lm[0][0], lm[1][1], lm[2][2])) * p;
      
      center += glm::vec3(p);
    }
    center /= entities.size();
  }

  return center;
}


EntityHandle SceneHierarchy::next(EntityHandle entity, int32_t step) const {
  int32_t const nentities = static_cast<int32_t>(entities_.size());

  int32_t next_id = entity->index() + step;
  next_id = (next_id < 0) ? nentities + next_id : next_id % nentities; //

  // [ O(n), improve ? ]
  for (auto e : entities_) {
    if (next_id-- == 0) {
      return e;
    }
  }
  return nullptr;
}

void SceneHierarchy::renderDebugRigs() const {
  for (auto e : frame_.drawables) {
    auto &visual = e->get<VisualComponent>();
    if (auto rig = visual.rig(); rig) {
      renderDebugNode(rig->child(0));
    }
  }
}

void SceneHierarchy::renderDebugColliders() const {
  for (auto e : frame_.colliders) {
    auto const& bsphere = e->get<SphereColliderComponent>();
    auto pos = globalMatrix(e->index()) * glm::vec4(bsphere.center(), 1.0);
    Im3d::PushColor( Im3d::Color(glm::vec4(0.0, 1.0, 1.0, 0.95)) );
    Im3d::DrawSphere( glm::vec3(pos), bsphere.radius(), kDebugSphereResolution);
    Im3d::PopColor();
  } 
}

void SceneHierarchy::gizmos(bool use_centroid) {
  // 1) Use global matrices for gizmos.
  for (auto e : selected()) {
    auto &global = globalMatrix( e->index() );
    auto const& centroid = e->centroid();

    if (use_centroid) {
      global = glm::translate( global, centroid);
    }
    Im3d::Gizmo( e->name().c_str(), glm::value_ptr(global)); // [modify global]
    if (use_centroid) {
      global = glm::translate( global, -centroid);
    }
  }
  
  // 2) Recompute selected locals from modified globals.
  updateSelectedLocalMatrices();
}

// ----------------------------------------------------------------------------

void SceneHierarchy::updateHierarchy(float const dt) {
  // (the root is used as a virtual entity and is hence not updated).
  int32_t index = -1;

  frame_.matrices_stack.push( glm::mat4(1.0f) );
  for (auto child : root_->children_) {
    ++index;
    subUpdateHierarchy(dt, child, index);
  }
  frame_.matrices_stack.pop();    
}

void SceneHierarchy::subUpdateHierarchy(float const dt, EntityHandle entity, int &index) {
  // Update entity.
  entity->index_ = index;
  entity->update(dt);

  // Update global matrix.
  auto& global_matrix{ frame_.globals[index] };
  global_matrix = frame_.matrices_stack.top() 
                * entity->transform().matrix()
                ;

  // Sub-hierarchy update.
  frame_.matrices_stack.push( global_matrix );
  for (auto child : entity->children_) {
    ++index;
    subUpdateHierarchy(dt, child, index);
  }
  frame_.matrices_stack.pop();
}

void SceneHierarchy::updateSelectedLocalMatrices() {
  // [slight bug with hierarchical multiselection : the update might use the non updated
  //  global of the parent, acting like a local transform]

  for (auto e : frame_.selected) {
    auto &local = e->localMatrix();
    auto const& global = globalMatrix( e->index() );

    auto p = e->parent();
    glm::mat4 inv_parent{1.0f};
    
    if (p && (p->index() > -1)) {
      auto const& parent_global{ globalMatrix(p->index()) };
      inv_parent = glm::affineInverse( parent_global );
    }
    local = inv_parent * global;
  }
}

void SceneHierarchy::sortDrawables(Camera const& camera) {
  auto const& eye_pos = camera.position();
  auto const& eye_dir = camera.direction();

  // Calculate the dot product of an entity to the camera direction.
  auto calculate_entity_dp = [this, eye_pos, eye_dir](EntityHandle const& e) {
    auto const pos = globalCentroid(e);
    auto const dir = pos - eye_pos;
    return glm::dot(eye_dir, dir);
  };

  // Store all the drawables dot products.
  std::vector<float> dotproducts;
  dotproducts.reserve(frame_.drawables.size());
  for (auto &e : frame_.drawables) {
    dotproducts.push_back(calculate_entity_dp(e));
  }

  // Sort drawables front to back.
  frame_.drawables.sort([&dotproducts](auto const& A, auto const& B) {
      return dotproducts[A->index()] < dotproducts[B->index()];
    }
  );
}

void SceneHierarchy::renderDebugNode(EntityHandle node) const {
  if (nullptr == node) {
    return;
  }

  {
    auto const n = node->nchildren();

    // Color.
    auto rgb = Im3d::Color((0 == n) ? glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) :    // leaf
                           (1 == n) ? glm::vec4(0.5f, 1.0f, 0.5f, 1.0f) :    // joint
                                      glm::vec4(1.0f, 1.0f, 0.9f, 1.0f) );   // node

    // Position.
    auto const start{ globalPosition(node) };

    // [fixme] the debug shapes should scale depending on some factors.
    float constexpr scale = 0.02f; //

    Im3d::PushColor( rgb );
    if (1 == n) {
      auto const end{ globalPosition(node->child(0)) };
      Im3d::DrawPrism( start, end, 1.0f * scale, 5);
    } else {
      Im3d::DrawSphere( start, 2.0f * scale, kDebugSphereResolution);
    }
    Im3d::PopColor();
  }

  // Recursively render sub hierarchy.
  for (auto &child : node->children()) {
    renderDebugNode(child);
  }
}

// ----------------------------------------------------------------------------
