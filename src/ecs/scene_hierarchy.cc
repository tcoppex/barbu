#include "ecs/scene_hierarchy.h"

#include <algorithm>
#include <vector>

#include "glm/gtc/matrix_inverse.hpp"
#include "im3d/im3d.h"  // (used for debug render)

#include "core/camera.h"
#include "ui/views/ecs/SceneHierarchyView.h"
#include "core/global_clock.h"

// ----------------------------------------------------------------------------

// Default name given to rig entity loaded from model.
static constexpr char const* kDefaultRigEntityName{ "[rig]" };

// Default name given to bounding sphere entities.
static constexpr char const* kDefaultBoundingSphereEntityName{ "BSphere" };

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
  float const global_time = GlobalClock::Get().application_time();

  // Clear per-frame data.
  frame_.clear();
  frame_.globals.resize(entities_.size(), glm::mat4(1.0f));

  // Update the scene entities hierarchically.
  update_hierarchy(dt);

  // Retrieve renderable entities.
  for (auto& e : entities_) {
    if (is_selected(e)) {
      frame_.selected.push_back( e );
    }

    if (e->has<VisualComponent>()) {
      frame_.drawables.push_back( e );
    }

    if (e->has<SphereColliderComponent>()) {
      frame_.colliders.push_back( e );
    }
  }

  // Sort the drawables front to back.
  sort_drawables(camera);

  // Animate nodes with skinning (for now, suppose them all drawables).
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
        auto const& rig_global = global_matrix(rig->index());
        auto const& controller = skin.controller();
        auto const& global_pose_matrices = controller.global_pose_matrices();

        // Map skeleton joint index to their rig entity.
        auto &skeleton_map = skin.skeleton_map(); //
        assert(!skeleton_map.empty());

        for (int32_t joint_id = 0; joint_id < controller.njoints(); ++joint_id) {
          auto const& global_pose = global_pose_matrices[joint_id];
          auto const entity_index = skeleton_map[joint_id]->index(); //
          assert( entity_index > -1 );

          //LOG_MESSAGE(entity_index);
          frame_.globals[entity_index] = rig_global * global_pose;
        }
      }
    }
  }
}

void SceneHierarchy::add_entity(EntityHandle entity, EntityHandle parent) {
  assert( nullptr != entity );
  assert( nullptr == entity->parent_ );

  if (nullptr == parent) {
    parent = root_;

    // [debug only] 
    //parent = entities_.empty() ? root_ : entities_.back();
  }

  entity->parent_ = parent;
  parent->children_.push_back( entity );
  entities_.push_back( entity );
}

void SceneHierarchy::remove_entity(EntityHandle entity, bool bRecursively) {
  if (nullptr == entity) {
    return;
  }

  // [todo] Make sure the assets are properly released when not used anymore.
  
  // Remove rig from loaded model. 
  if (entity->has<VisualComponent>()) {
    auto visual = entity->get<VisualComponent>();
    if (auto rig = visual.rig(); rig) {
      remove_entity( rig, true);
      visual.set_rig(nullptr);
    }
  } else if(entity->name() == kDefaultRigEntityName) {
    if (auto parent = entity->parent(); parent && parent->has<VisualComponent>()) {
      parent->get<VisualComponent>().set_rig(nullptr);
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
      remove_entity(child, bRecursively);
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

void SceneHierarchy::reset_entity(EntityHandle entity, bool bRecursively) {
  if (nullptr == entity) {
    return;
  }

  // Remove children recursively.
  if (bRecursively) {
    for (auto &child : entity->children_) {
      reset_entity(child, bRecursively);
    }
  }

  // Reset transform.
  entity->transform().reset();
}

EntityHandle SceneHierarchy::import_model(std::string_view filename) {
  /// This load the whole geometry of the file as a single mesh.
  /// [ create an importer for scene structure. ]

  // When successful, add a new model entity to the scene.
  if (auto mesh = MESH_ASSETS.create( AssetId(filename) ); mesh && mesh->loaded()) {
    // Import materials separetely.
    MATERIAL_ASSETS.import_from_meshdata( ResourceId(filename) );

    // Retrieve the file basename.
    auto const basename = Resource::TrimFilename( std::string(filename) );    
    
    // Create a new mesh entity node.
    auto entity = create_model_entity(basename, mesh);

    // Handle skinned model.
    if (entity) {
      if (auto skl = entity->as<ModelEntity>().skeleton(); skl) {
        // ----------------------------
        // [ Work In Progress]

        // Add a skinning component.
        auto& skin = entity->add<SkinComponent>();
        skin.set_skeleton( skl );

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
          auto rig_root = std::make_shared<Entity>( kDefaultRigEntityName ); 
          add_entity( rig_root, entity);

          // Add the rig root to the visual component.
          auto &visual = entity->get<VisualComponent>(); 
          visual.set_rig( rig_root );

          // We use a map intern to the skin component to find entities from their
          // joint index.
          auto const njoints{ skl->njoints() };
          auto &skeleton_map = skin.skeleton_map();  //
          skeleton_map.resize( njoints );
          
          // Add joint entities to the hierarchy.
          for (int32_t i = 0; i < njoints; ++i) {
            auto joint_entity = std::make_shared<Entity>( skl->names[i] );
            
            // Add the entity to its parent in the hierarchy.
            auto const parent_id = skl->parents[i];
            add_entity( joint_entity, (parent_id > -1) ? skeleton_map[parent_id] : rig_root);

            // Update the entity local matrix using skeleton matrices.
            auto const& parent_inv_matrix = (parent_id > -1) ? skl->inverse_bind_matrices[parent_id] : glm::mat4(1.0f);
            joint_entity->local_matrix() = parent_inv_matrix * skl->global_bind_matrices[i];

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

void SceneHierarchy::update_selected_local_matrices() {
  // [slight bug with hierarchical multiselection : the update might use the non updated
  //  global of the parent, acting like a local transform]

  for (auto e : frame_.selected) {
    auto &local = e->local_matrix();
    auto const& global = global_matrix( e->index() );

    auto p = e->parent();
    glm::mat4 inv_parent{1.0f};
    
    if (p && (p->index() > -1)) {
      auto const& parent_global{ global_matrix(p->index()) };
      inv_parent = glm::affineInverse( parent_global );
    }
    local = inv_parent * global;
  }
}

void SceneHierarchy::select(EntityHandle entity, bool status) {
  // [fixme] using the UI as state holder is not great.
  ui_view->select(entity->index(), status);
}

void SceneHierarchy::select_all(bool status) {
  ui_view->select_all(status);
}

bool SceneHierarchy::is_selected(EntityHandle entity) const {
  return entity ? ui_view->is_selected(entity->index()) : false;
}

glm::vec3 SceneHierarchy::pivot(bool selected) const {
  auto const& entities = (selected && !frame_.selected.empty()) ? frame_.selected : entities_;

  glm::vec3 pivot{ 0.0f };
  if (!entities.empty()) {
    for (auto const& e : entities) {
      pivot += entity_global_position(e);
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
      auto const& lm = e->local_matrix(); 
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

EntityHandle SceneHierarchy::add_bounding_sphere(float radius) {
#if 0
  if (auto mesh = MESH_ASSETS.createSphere(kDebugSphereResolution, kDebugSphereResolution); mesh) {
    return create_model_entity( kDefaultBoundingSphereEntityName, mesh);
  }
#else
  auto entity = std::make_shared<Entity>(kDefaultBoundingSphereEntityName);
  if (entity != nullptr) {
    add_entity(entity);
    auto &bsphere = entity->add<SphereColliderComponent>();
    bsphere.set_radius(radius);
  }
#endif

  return nullptr;
}

void SceneHierarchy::render_debug_rigs() const {
  for (auto e : frame_.drawables) {
    auto &visual = e->get<VisualComponent>();
    if (auto rig = visual.rig(); rig) {
      render_debug_node(rig->child(0));
    }
  }
}

void SceneHierarchy::render_debug_colliders() const {
  for (auto e : frame_.colliders) {
    auto const& bsphere = e->get<SphereColliderComponent>();
    auto pos = global_matrix(e->index()) * glm::vec4(bsphere.center(), 1.0);
    Im3d::PushColor( Im3d::Color(glm::vec4(0.0, 1.0, 1.0, 0.95)) );
    Im3d::DrawSphere( glm::vec3(pos), bsphere.radius(), kDebugSphereResolution);
    Im3d::PopColor();
  } 
}

void SceneHierarchy::gizmos(bool use_centroid) {
  // 1) Use global matrices for gizmos.
  for (auto e : selected()) {
    auto &global = global_matrix( e->index() );
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
  update_selected_local_matrices();
}

// ----------------------------------------------------------------------------

EntityHandle SceneHierarchy::create_model_entity(std::string const& basename, MeshHandle mesh) {
  // Create a unique entity.
  auto entity = std::make_shared<ModelEntity>( basename, mesh);

  // Add it to the scene hierarchy.
  if (entity != nullptr) {
    add_entity(entity);
  }
  return entity;
}

void SceneHierarchy::update_hierarchy(float const dt) {
  // (the root is used as a virtual entity and is hence not updated).
  int32_t index = -1;

  frame_.matrices_stack.push( glm::mat4(1.0f) );
  for (auto child : root_->children_) {
    ++index;
    update_sub_hierarchy(dt, child, index);
  }
  frame_.matrices_stack.pop();    
}

void SceneHierarchy::update_sub_hierarchy(float const dt, EntityHandle entity, int &index) {
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
    update_sub_hierarchy(dt, child, index);
  }
  frame_.matrices_stack.pop();
}

void SceneHierarchy::sort_drawables(Camera const& camera) {
  auto const& eye_pos = camera.position();
  auto const& eye_dir = camera.direction();

  // Calculate the dot product of an entity to the camera direction.
  auto calculate_entity_dp = [this, eye_pos, eye_dir](EntityHandle const& e) {
    auto const pos = entity_global_centroid(e);
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

void SceneHierarchy::render_debug_node(EntityHandle node) const {
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
    auto const start{ entity_global_position(node) };

    // [fixme] the debug shapes should scale depending on some factors.
    double constexpr scale = 0.02; //

    Im3d::PushColor( rgb );
    if (1 == n) {
      auto const end{ entity_global_position( node->child(0) ) };
      Im3d::DrawPrism( start, end, 1.0 * scale, 5);
    } else {
      Im3d::DrawSphere( start, 2.0 * scale, kDebugSphereResolution);
    }
    Im3d::PopColor();
  }

  // Recursively render sub hierarchy.
  for (auto &child : node->children()) {
    render_debug_node(child);
  }
}

// ----------------------------------------------------------------------------
