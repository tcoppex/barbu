#ifndef BARBU_MEMORY_RESOURCES_RESOURCES_H_
#define BARBU_MEMORY_RESOURCES_RESOURCES_H_

#include "memory/resources/image.h"
#include "memory/resources/mesh_data.h"
#include "memory/resources/shader.h"

// ----------------------------------------------------------------------------

//
//  'Static' class to wrap all the resources loaders.
//
// @note :  * Put the file modification watcher in a thread.
//          * UpdateAll should ideally not be called every frames and have different
//      elapsed time by resource type.
//
class Resources final {
 public:
  static void UpdateAll() {
    sImage.update();
    sMeshData.update();
    sShader.update();
  }

  static void ReleaseAll() {
    sImage.release_all();
    sMeshData.release_all();
    sShader.release_all();
  }


  template<typename T>
  static bool Has( ResourceId const& id );

  template<typename T>
  static typename ResourceManager<T>::Handle Get( ResourceId const& id );

  template<typename T>
  static typename ResourceManager<T>::Handle GetUpdated( ResourceInfo & info );

  template<typename T>
  static ResourceId Add( std::string const& basename, T const& resource ); //

  template<typename T>
  static bool CheckVersion(ResourceInfo const& info);

 private:
  static ImageManager sImage;
  static MeshDataManager sMeshData;
  static ShaderManager sShader;
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_RESOURCE_RESOURCES_H_