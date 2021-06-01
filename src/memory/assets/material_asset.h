#ifndef BARBU_MEMORY_ASSETS_MATERIAL_ASSET_H_
#define BARBU_MEMORY_ASSETS_MATERIAL_ASSET_H_

#include "memory/asset_factory.h"
#include "memory/resources/mesh_data.h"

class Material;
using MaterialHandle = std::shared_ptr<Material>;

// ----------------------------------------------------------------------------

struct MaterialAssetParameters : AssetParameters {
  MaterialAssetParameters() = default;

  MaterialAssetParameters(ResourceId meshdata_id, int32_t _index = -1)
    : AssetParameters( {meshdata_id} )
    , index(_index)
  {}

  int32_t index = -1;
};

// ----------------------------------------------------------------------------

//
//  MaterialAssets holds an internal Material (see ecs/material.h) and depends
// on a MeshData resource, as for Mesh Assets.
//
class MaterialAsset : public Asset<MaterialAssetParameters, MeshData> {
 public:
  MaterialAsset(Parameters_t const& _params)
    : Asset(_params)
  {}

  inline bool loaded() const noexcept override { 
    return material_ != nullptr;
  }

  inline MaterialHandle get() { 
    return material_;
  }

 private:
  void allocate() override;
  void release() override;
  bool setup() override;

  MaterialHandle material_;

  template<typename> friend class AssetFactory;
};

// ----------------------------------------------------------------------------

using MaterialAssetHandle = AssetHandle<MaterialAsset>;

// ----------------------------------------------------------------------------

class MaterialAssetFactory : public AssetFactory<MaterialAsset> {
 public:
  MaterialAssetFactory() {
    bReleaseUniqueAssets_ = false;
  }

  ~MaterialAssetFactory() { 
    release_all();
  }

  // Return an handle to the default material.
  Handle get_default();

  // Import all materials from the specific meshdata resource.
  void import_from_meshdata(ResourceId meshdata_id);

 private:
  Handle default_material_asset_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_ASSETS_MATERIAL_ASSET_H_
