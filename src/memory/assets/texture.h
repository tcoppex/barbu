#ifndef BARBU_MEMORY_ASSETS_TEXTURE_H_
#define BARBU_MEMORY_ASSETS_TEXTURE_H_

#include "memory/asset_factory.h"
#include "memory/resources/image.h"

namespace views {
class TexturesView;
}

// ----------------------------------------------------------------------------

struct TextureParameters : AssetParameters {
  int32_t target = 0;
  int32_t levels = 0;
  int32_t internalFormat = 0;
  int32_t w = 0;
  int32_t h = 0;
  int32_t depth = 0;
  void* pixels = nullptr;
};

// ----------------------------------------------------------------------------

struct Texture : Asset<TextureParameters, Image> {
 private:
  // [ not covered everywhere ]
  // When true and hot-reload is activated, setup can destroy the current texture 
  // to change its resolution (all storage are immutables).
  // Otherwhise the texture is reloaded only when the resolution has not changed.
  static constexpr bool kImmutableResolution = false;

 public:
  // Return the maximum mip map level for a 2d texture.
  static int32_t GetMaxMipLevel(int32_t res) {
    return static_cast<int32_t>(glm::log(res) * 1.4426950408889634);
  }
  static int32_t GetMaxMipLevel(int32_t w, int32_t h) {
    return GetMaxMipLevel(glm::min(w, h));
  }

  Texture(Parameters_t const& _params)
    : Asset(_params)
  {}
  
  ~Texture() { release(); }

  bool loaded() const noexcept final {
    return id != 0u;
  }

  void generate_mipmaps();

  int32_t levels() const { return params.levels; }
  int32_t width() const { return params.w; }
  int32_t height() const { return params.h; }

  uint32_t id = 0u; //

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
  Handle create2d(AssetId const& id, int levels, int internalFormat, int w, int h, void *pixels = nullptr);   // internal with params

  // Texture 3d
  Handle create3d(AssetId const& id, int levels, int internalFormat, int w, int h, int d, void *pixels = nullptr);
  Handle create3d(AssetId const& id, int levels, int internalFormat, int res, void *pixels = nullptr) {
    return create3d( id, levels, internalFormat, res, res, res, pixels);
  }

  // Cubemap.
  Handle createCubemap(AssetId const& id, int levels, int internalFormat, ResourceInfoList const& dependencies);
  Handle createCubemap(AssetId const& id, ResourceInfoList const& dependencies);
  Handle createCubemap(AssetId const& id, int levels, int internalFormat, int w, int h, void *pixels = nullptr);
  // Crossed HDR Cubemap.
  Handle createCubemapHDR(AssetId const& id, int levels, ResourceId const& resource = nullptr);

  friend class views::TexturesView;
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_ASSETS_TEXTURE_H_
