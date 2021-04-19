#ifndef BARBU_MEMORY_ASSETS_TEXTURE_H_
#define BARBU_MEMORY_ASSETS_TEXTURE_H_

#include "memory/asset_factory.h"
#include "memory/resources/image.h"

// ----------------------------------------------------------------------------

struct TextureParameters : AssetParameters {
  int32_t target = 0;
  int32_t levels = 0;
  int32_t internalFormat = 0;
  int32_t w = 0;
  int32_t h = 0;
  int32_t depth = 0;
};

// ----------------------------------------------------------------------------

struct Texture : Asset<TextureParameters, Image> {
 private:
  // When true and hot-reload is activated, setup can destroy the current texture 
  // to change its resolution (storage are immutable).
  // Otherwhise the texture is reloaded only when the resolution has not changed.
  static constexpr bool kImmutableResolution = false;

 public:
  Texture(Parameters_t const& _params)
    : Asset(_params)
  {}
  
  ~Texture() { release(); }

  bool loaded() const noexcept final {
    return id != 0u;
  }

  uint32_t id = 0u;

 private:
  void allocate() final;
  void release() final;
  bool setup() final;

  template<typename> friend class AssetFactory;
};

// ----------------------------------------------------------------------------

using TextureHandle = AssetHandle<Texture>;

// ----------------------------------------------------------------------------

class TextureFactory : public AssetFactory<Texture> {
 public:
  ~TextureFactory() { release_all(); }

  // Texture 2d.
  Handle create2d(AssetId const& id, int levels, int internalFormat, ResourceId const& resource = nullptr);   // external with params
  Handle create2d(AssetId const& id, ResourceId const& resource = nullptr);                                   // external with defaults
  // Handle create2d(AssetId const& id, int levels, int internalFormat, int w, int h);                           // internal with params

  // Cubemap.
  Handle createCubemap(AssetId const& id, int levels, int internalFormat, ResourceInfoList const& dependencies);
  Handle createCubemap(AssetId const& id, ResourceInfoList const& dependencies);
  // Crossed HDR Cubemap.
  Handle createCubemapHDR(AssetId const& id, ResourceId const& resource = nullptr);
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_ASSETS_TEXTURE_H_
