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

bool SceneHierarchy::import_model(std::string_view filename) {
/*

  # Protocol idea for loading a mesh materials.

  When we import a mesh from a file, a list of material can be found in two way :
   1) it is set internally in the file, or
   
   2) it got a reference to another file.

  In the two cases we can immediately load the material list and create in a separate
  factory each materials as a different asset with a unique AssetId of the type :
  "MaterialFileBaseName::MaterialName" where MaterialFileBaseName is the basename
  of the material when it exists or the model's name otherwhise.

  Each mesh containing materials has a set of vertex groups referencing the material
  names and indices ranges. When an entity is created it contains an empty map of
  the size of the number of materials it possesses indexed by their names
  (the same as in the vertex group) and when empty referred directly to the default
  materials contained in the factory. An entity material can thus be modified when
  it is not the default one [ala Unity].

  Questions :
  - How do we transfer the MaterialFileBaseName to the mesh ?
    Possible solution : change the material name to the whole shebang directly.
    -> Default materials are load at the same time as the mesh but its vertex groups are
    rename from "vgroupname" to "materialFileBaseName::vgroupname" for example, which is also
    the material HashId in the material factory. This more or less prevents collision in the factory.

  - This system means that the MeshData Resource probably creates the Material "Assets".
  Is it feasible dependency-wise ? is it acceptable ?
    -> The resource could not create an asset directly but I don't think a Material is a pure Asset 
     as I defined them so it would just use a different kind of factory.
    -> Alternatively the meshData resource holds the material excerpt and the
     MaterialHandle is created afterwards from it, either from here or from "Mesh" for example.

    It might be better if "SceneHierarchy::import" calls upon a MeshData resource and create the
    underlying assets as needed, for example the 1-to-n mesh entities (when using multiple objects)
    and their materials. 
     Who has to deal with dependencies ? 
      -> still the assets, with added parameters.
                           |- Mesh_1
    Resource -> Entity -> { - Mesh_2
                           |- Material_i
      -> In this case there is two ways to load an object : 
        * from MESHES which would automatically join all sub-objects, or
        * from the scene hierarchy which would allow to subdivide the mesh per object 
        (with info in asset parameters).

  -> It is not a "mesh data file", it is a "scene file" with multiple sub-data.

  To do :
    - Material Factory (maybe not an asset ?)
*/

  AssetId const asset_id( filename );

  // When successful, add a new model entity to the scene.
  if (auto mesh = MESH_ASSETS.create( asset_id ); mesh && mesh->loaded()) {
    // Import materials separetely.
    MATERIAL_ASSETS.import( ResourceId(filename) );

    // Retrieve the file basename.
    auto const basename = Resource::TrimFilename( std::string(filename) );
    
    // Create a unique entity.
    auto entity = std::make_shared<ModelEntity>( basename, mesh);

    // Add it to the scene hierarchy.
    add_entity( entity );

    return true;
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

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

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
