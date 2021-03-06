#ifndef BARBU_ECS_COMPONENTS_VISUAL_H_
#define BARBU_ECS_COMPONENTS_VISUAL_H_

#include <map>

#include "ecs/component.h"
#include "ecs/materials/generic.h"
#include "memory/assets/mesh.h"

#include "ecs/entity-fwd.h"

// ----------------------------------------------------------------------------

//
// Store rendering properties and link submeshes with their material. 
//
class VisualComponent final : public ComponentParams<Component::Visual> {
 public:
  VisualComponent() = default;

  // Render parts of the mesh matching the RenderMode.
  void render(RenderAttributes const& attributes, RenderMode render_mode) {
    if (!mesh_->has_materials()) {
      // Case 1 : Mesh with no materials.

      if (render_mode == RenderMode::kDefault) {
        material()->update_uniforms(attributes);
        mesh_->draw();
      }
    } else {
      // Case 2 : Mesh with sub materials.

      // Keep tracks of previous program to avoid useless uniform setup mesh wised.
      uint32_t last_pgm = 0u;
      int32_t texture_unit = 0;

      auto const nsubgeometry = mesh_->nsubgeometry();
      for (auto i = 0; i < nsubgeometry; ++i) {
        auto const mat = material(i);
        assert(mat != nullptr);

        // Check the material render mode.
        if (render_mode != mat->render_mode()) {
          continue;
        }

        // Set material parameters when needed.
        uint32_t const pgm = mat->program()->id;
        bool const bUseSameProgram = (last_pgm == pgm); 
        texture_unit = mat->update_uniforms(attributes, bUseSameProgram ? texture_unit : 0);
        last_pgm = pgm;

        // Force double-sided rendering when requested.
        bool const bCullFace = gx::IsEnabled( gx::State::CullFace );
        if (mat->double_sided()) {
          gx::Disable( gx::State::CullFace );
        }

        // Draw submesh.
        mesh_->draw_submesh(i);

        // Restore pipeline state.
        if (bCullFace) {
          gx::Enable( gx::State::CullFace );
        }
      }
    }

    CHECK_GX_ERROR();
  }

  // Add a mesh with a default material for each submeshes.
  inline void set_mesh(MeshHandle mesh) {
    mesh_ = mesh;

    // materials_.clear();
    // for (auto& vg : mesh_->vertex_groups()) {
    //   auto mat = MATERIAL_ASSETS.get( AssetId(vg.name) );
    //   materials_.insert_or_assign( vg.name, mat->loaded() ? mat : MATERIAL_ASSETS.get_default());
    // }
  }

  inline void set_rig(EntityHandle rig) {
    rig_ = rig;
  }

  inline MeshHandle mesh() { 
    return mesh_; 
  }

  inline MeshHandle mesh() const { 
    return mesh_; 
  }

  inline EntityHandle rig() const {
    return rig_;
  }

 private:
  inline MaterialHandle material(int32_t index=0) {
    auto const default_material = MATERIAL_ASSETS.get_default()->get();

    if (!mesh_->has_materials()) {
      return default_material;
    }
    auto const& vg = mesh_->vertex_group(index);
    auto const material_id = AssetId(vg.name);

    return (MATERIAL_ASSETS.has(material_id)) ?
      MATERIAL_ASSETS.get(material_id)->get() :
      default_material
    ;
  }

  MeshHandle mesh_ = nullptr; 

  // If the entity is skinned, reference its rig here.
  EntityHandle rig_ = nullptr;

  // [right now the mesh access its linked materials at import but we could use
  // an internal map to access custom one]
  //std::map< std::string, MaterialAssetHandle > materials_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_COMPONENTS_VISUAL_H_