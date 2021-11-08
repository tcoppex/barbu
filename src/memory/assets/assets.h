#ifndef BARBU_MEMORY_ASSETS_ASSETS_H_
#define BARBU_MEMORY_ASSETS_ASSETS_H_

#include "memory/assets/texture.h"
#include "memory/assets/mesh.h"
#include "memory/assets/program.h"
#include "memory/assets/material_asset.h"

// ----------------------------------------------------------------------------

//
//  'Static' class to wrap all the assets loaders.
//
class Assets final {
 public:
  static void UpdateAll() {
    sTexture.update();
    sMesh.update();
    sProgram.update();
    sMaterialAsset.update();
  }

  static void ReleaseAll() {
    sTexture.release_all();
    sMesh.release_all();
    sProgram.release_all();
    sMaterialAsset.release_all();
  }

  static TextureFactory       sTexture;
  static MeshFactory          sMesh;
  static ProgramFactory       sProgram;
  
  static MaterialAssetFactory sMaterialAsset;
};

#define TEXTURE_ASSETS        (Assets::sTexture)
#define MESH_ASSETS           (Assets::sMesh)
#define PROGRAM_ASSETS        (Assets::sProgram)
#define MATERIAL_ASSETS       (Assets::sMaterialAsset)

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_RESOURCE_RESOURCES_H_