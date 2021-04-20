#ifndef BARBU_ECS_COMPONENTS_VISUAL_H_
#define BARBU_ECS_COMPONENTS_VISUAL_H_

#include <map>
#include "ecs/component.h"
#include "ecs/materials/generic.h"
#include "memory/assets/mesh.h"

// ----------------------------------------------------------------------------

//
// Link submeshes with materials and store basic rendering info. 
//
// The VisualComponent is responsible to register its material to the
// renderer pass manager.
//
class VisualComponent final : public ComponentParams<Component::Visual> {
 public:
  VisualComponent() = default;

  // Render parts of the mesh matching the RenderMode.
  void render(RenderAttributes const& attributes, RenderMode render_mode) {
    if (!mesh_->has_materials()) 
    {
    // Case 1 : Mesh with no materials.
      auto mat = MATERIAL_ASSETS.get_default()->get();
      if (render_mode == mat->render_mode()) {
        mat->update_uniforms(attributes);
        mesh_->draw();
      }
    } 
    else 
    {
    // Case 2 : Mesh with submaterials.

      // Keep track of previous pgm to avoid useless uniform setup (could be improved).
      uint32_t last_pgm = 0u;

      auto const nsubgeometry = mesh_->nsubgeometry();
      for (auto i = 0; i < nsubgeometry; ++i) {
        auto const& vg = mesh_->vertex_group(i);
        auto const material_id = AssetId(vg.name); //
        //LOG_INFO("material : ", material_id.c_str(), MATERIAL_ASSETS.has(material_id));

        // If the material exists for the mesh, find it, otherwhise take the default material.
        auto mat = (MATERIAL_ASSETS.has(material_id)) ?
          MATERIAL_ASSETS.get(material_id)->get() :
          MATERIAL_ASSETS.get_default()->get()
        ;

        // Check the material render mode.
        if (render_mode != mat->render_mode()) {
          continue;
        }

        // Set material parameters when needed.
        uint32_t const pgm = mat->program()->id;
        mat->update_uniforms(attributes, last_pgm == pgm);
        last_pgm = pgm;

        // Draw submesh.
        mesh_->draw_submesh(i);
        //CHECK_GX_ERROR();
      }
    }
  }

  // Add a mesh with a default material for each submeshes.
  void set_mesh(MeshHandle mesh) {
    mesh_ = mesh;

    // materials_.clear();
    // for (auto& vg : mesh_->vertex_groups()) {
    //   auto mat = MATERIAL_ASSETS.get( AssetId(vg.name) );
    //   materials_.insert_or_assign( vg.name, mat->loaded() ? mat : MATERIAL_ASSETS.get_default());
    // }
  }

  inline MeshHandle mesh() { 
    return mesh_; 
  }

 private:
  MeshHandle mesh_; 
  //std::map< std::string, MaterialAssetHandle > materials_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_COMPONENTS_VISUAL_H_