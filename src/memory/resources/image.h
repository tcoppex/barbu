#ifndef BARBU_MEMORY_RESOURCES_IMAGE_H_
#define BARBU_MEMORY_RESOURCES_IMAGE_H_

#include "memory/resource_manager.h"

// ----------------------------------------------------------------------------

struct Image : public Resource {
  int width    = 0;
  int height   = 0;
  int depth    = 0;
  int channels = 0;
  void* pixels = nullptr;
  bool hdr     = false;

  ~Image() {
    release();
  }

  void release() final;

  bool loaded() const noexcept final {
    return nullptr != pixels;
  }
};

// ----------------------------------------------------------------------------

class ImageManager : public ResourceManager<Image> {
 private:
  Handle _load(ResourceId const& id) final;

  Handle _load_internal(ResourceId const& id, int32_t size, void const* data, std::string_view mime_type) final;

  // Transform internal data of a crossed hdr to an array of cube faces.
  void setup_crossed_hdr(Image &img);
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_RESOURCES_IMAGE_H_