#include "ecs/scene_hierarchy.h"

#include <algorithm>
#include <vector>

#include "glm/gtc/matrix_inverse.hpp"

#include "core/camera.h"
#include "ui/views/ecs/SceneHierarchyView.h"
#include "core/global_clock.h"

// ----------------------------------------------------------------------------

// Default name given to rig entity loaded from model.
static constexpr char const* kDefaultRigEntityName{ "[rig]" };

constexpr bool kEnableLoadRigHierarchy = true;

// ----------------------------------------------------------------------------

SceneHierarchy::~SceneHierarchy() {
  if (ui_view) {
    delete ui_view;
    ui_view = nullptr;
  }
}

void SceneHierarchy::init() {
  ui_view = new views::SceneHierarchyView(*this); //
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
  }

  // Sort the drawables front to back.
  sort_drawables(camera);

  // Animate nodes with skinning (for now, suppose them all drawables).
  for (auto& e : frame_.drawables) {
    if (e->has<SkinComponent>()) {
      e->get<SkinComponent>().update(global_time);
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

  // Children.
  if (bRecursively) {
    // Remove them recursively.
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

  // Children.
  if (bRecursively) {
    // Remove them recursively.
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
        // Add a skin component to the entity.
        {
          auto& skin = entity->add<SkinComponent>();
          skin.set_skeleton( skl );

          // [it's important to use references, for pointer validity... XXX]
          if (auto& clips = skl->clips; !clips.empty()) {
            clips[0].bLoop = true; // [debug]
            auto& sequence = skin.sequence();
            sequence.push_back( SequenceClip_t(&clips[0]) );
          } else {
            LOG_WARNING( "no clips for skinned entity", basename );
          }
        }
        // ----------------------------

        // Add a mockup rig as sub hierarchy.  
        if constexpr (kEnableLoadRigHierarchy) {
  
          // Note : 
          // This could disrupt the entity focus feature.

          auto const njoints{ skl->njoints() };
          std::vector<EntityHandle> rig( njoints );

          // Calculate the bones globals binds.
          // ( costly as it inverts the inverse global binds )
          skl->calculate_global_bind_matrices();

          // Create an upper rig entity.
          EntityHandle rig_root = std::make_shared<Entity>( kDefaultRigEntityName ); 
          add_entity( rig_root, entity);

          // Add the rig root to the visual component.
          entity->get<VisualComponent>().set_rig( rig_root );

          // Add hierarchy.
          for (int i = 0; i < njoints; ++i) {
            rig[i] = std::make_shared<Entity>( skl->names[i] );
            
            auto const parent_id = skl->parents[i];
            add_entity( rig[i], (parent_id > -1) ? rig[parent_id] : rig_root);

            auto const& parent_inv_matrix = (parent_id > -1) ? skl->inverse_bind_matrices[parent_id] : glm::mat4(1.0f);
            rig[i]->local_matrix() = parent_inv_matrix * skl->global_bind_matrices[i];
          }
        }
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

void SceneHierarchy::select_all(bool status) {
  // [fixme] technically, using the UI as state holder is not great.
  ((views::SceneHierarchyView*)ui_view)->select_all(status);
}

bool SceneHierarchy::is_selected(EntityHandle entity) const {
  return entity ? ((views::SceneHierarchyView*)ui_view)->is_selected(entity->index()) : false;
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

EntityHandle SceneHierarchy::add_bounding_sphere() {
  constexpr int32_t kDefaultRes = 16;
  if (auto mesh = MESH_ASSETS.createSphere(kDefaultRes, kDefaultRes); mesh) {
    return create_model_entity("BSphere", mesh);
  }
  return nullptr;
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
  int index = -1;

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

// ----------------------------------------------------------------------------
