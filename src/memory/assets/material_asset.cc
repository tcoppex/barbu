#include "memory/assets/material_asset.h"
#include "core/graphics.h"

#include "ecs/materials/generic.h"

// ----------------------------------------------------------------------------

void MaterialAsset::allocate() {
  assert( !loaded() );
  material_ = std::make_shared<GenericMaterial>(); //
}

void MaterialAsset::release() {
  if (loaded()) {
    material_.reset();
  }
}

bool MaterialAsset::setup() {
  assert( loaded() );
  
  if (auto &resources = params.dependencies; !resources.empty()) {
    auto meshdata = Resources::GetUpdated<Resource_t>( resources[0] ).data;
    if (!meshdata) {
      return false; 
    }

    auto matinfo = meshdata->material.get(params.index);
    material_->setup( matinfo );
  }
  
  return true;
}

// ----------------------------------------------------------------------------

MaterialAssetFactory::Handle MaterialAssetFactory::get_default() {
  AssetId const kDefaultId( MeshData::kDefaultGroupName );
  
  if (!default_material_asset_) {
    // Create the default material.
    default_material_asset_ = create( kDefaultId, MaterialAssetParameters() );
    // Setup with default material info.
    auto mat = default_material_asset_->get();
    mat->setup(MaterialInfo());
  }

  return default_material_asset_;
}
  
void MaterialAssetFactory::import_from_meshdata(ResourceId meshdata_id) {
  if (auto h = Resources::Get<MeshData>( meshdata_id ); h.is_valid() && h.data->has_materials()) { 
    int32_t index = 0;
    for (auto const& info : h.data->material.infos) {
      create( AssetId(info.name), Parameters_t(meshdata_id, index++));
    }
  }
}

// ----------------------------------------------------------------------------
