#include "ecs/scene_hierarchy.h"

#include <algorithm>
#include "glm/gtc/matrix_inverse.hpp"
#include "core/camera.h"
#include "ui/views/ecs/SceneHierarchyView.h"

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
  // Clear per-frame data.
  frame_.clear();
  frame_.globals.resize(entities_.size(), glm::mat4(1.0f));

  // Update the scene entities hierarchically.
  update_hierarchy(dt);

  // Retrieve renderable entities.
  for (auto& e : entities_) {
    if (((views::SceneHierarchyView*)ui_view)->is_selected(e->index())) {
      frame_.selected.push_back( e );
    }

    if (e->has<VisualComponent>()) {
      frame_.drawables.push_back( e );
    }
  }

  // Sort the drawables front to back.
  sort_drawables(camera);
}

void SceneHierarchy::add_entity(EntityHandle entity, EntityHandle parent) {
  assert( nullptr != entity );
  assert( nullptr == entity->parent_ );
  //assert( entity->children_.empty() );

  if (nullptr == parent) {
    parent = root_; 
             //entities_.empty() ? root_ : entities_.back();
  }

  entity->parent_ = parent;
  parent->children_.push_back( entity );
  entities_.push_back( entity );
}

void SceneHierarchy::remove_entity(EntityHandle entity, bool bRecursively) {
  if (nullptr == entity) {
    LOG_WARNING(__FUNCTION__, ": invalid parameters.");
    return;
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
    LOG_WARNING(__FUNCTION__, ": invalid parameters.");
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

bool SceneHierarchy::import_model(std::string_view filename) {
  /// This load the whole geometry of the file as a single mesh.
  /// TODO : create an importer for scene structure.

  // When successful, add a new model entity to the scene.
  if (auto mesh = MESH_ASSETS.create( AssetId(filename) ); mesh && mesh->loaded()) {
    // Import materials separetely.
    MATERIAL_ASSETS.import_from_meshdata( ResourceId(filename) );

    // Retrieve the file basename.
    auto const basename = Resource::TrimFilename( std::string(filename) );    
    
    // Create a new mesh entity node.
    auto entity = create_model_entity(basename, mesh);

    return entity != nullptr;
  }
  return false;
}

void SceneHierarchy::update_selected_local_matrices() {
  // [slight bug with hierarchical multiselection : the update might use the non updated
  //  global of the parent, acting like a local transform]

  for (auto e : frame_.selected) {
    auto &local = e->local_matrix();
    auto const& global = global_matrix( e->index() );

    // (right now the roots dont have globals, so we check its not the parent)
    auto p = e->parent();
    glm::mat4 inv_parent{1.0f};
    if (p && (p->index() > -1)) {
      auto const& parent_global{ global_matrix(p->index()) };
      inv_parent = glm::affineInverse( parent_global );
    }
    local = inv_parent * global;
  }
}

glm::vec3 SceneHierarchy::centroid(bool selected) const {
  glm::vec3 center{0.0f};

  if (selected && !frame_.selected.empty()) {
    for (auto const& e : frame_.selected) {
      center -= e->transform().position();
    }
    center /= frame_.selected.size();
  } else {
    for (auto const& e : entities_) {
      center -= e->transform().position();
    }
    center /= entities_.size();
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
  auto calculate_entity_dp = [eye_pos, eye_dir](EntityHandle const& e) {
    glm::vec3 pivot = glm::vec3(0.0f);
    if (auto visual = e->get<VisualComponent>(); e->has<VisualComponent>()) {
      pivot = visual.mesh()->pivot();
    }
    auto const& pos = e->transform().position() - pivot;
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
