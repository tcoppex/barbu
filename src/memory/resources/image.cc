#include "memory/resources/image.h"
#include <array>

// ----------------------------------------------------------------------------

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif // __GNUC__

#define STBI_NO_PSD
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

// ----------------------------------------------------------------------------

void Image::release() {
  if (pixels) {
    stbi_image_free(pixels);
    pixels = nullptr;
  }
}

ImageManager::Handle ImageManager::_load(ResourceId const& id) {
  ImageManager::Handle h(id);

  auto img = h.data;
  char const* filename = id.path.c_str();

  stbi_set_flip_vertically_on_load(false);

  if (stbi_is_hdr(filename)) {
    img->hdr = true;
    img->pixels = stbi_loadf(filename, &img->width, &img->height, &img->channels, kDefaultNumChannels); //
  } else {
    img->hdr = false;
    img->pixels = stbi_load( filename, &img->width, &img->height, &img->channels, kDefaultNumChannels); //
  }

  // Force the number of channels to maximum.
  // [ this could be overkill but it more or less assure stability across the pipeline ]
  img->channels = kDefaultNumChannels;
  img->depth = 1;

  if (nullptr == img->pixels) {
    LOG_WARNING( "Image Resource load failed for :", id.c_str());
    h.data.reset();
    h.data = nullptr;
  } else if (img->hdr) {
    // Reorganized HDR data to fit inside a cubemap.
    bool const is_crossed{id.path.find("cross") != std::string::npos};
    if (is_crossed) {
      setup_crossed_hdr(*img);
    } else {
      // [ transform equirectangular to cubemap here ? ]
    }
  }


  LOG_DEBUG_INFO(__FUNCTION__, id.c_str());

  return h;
}

ImageManager::Handle ImageManager::_load_internal(ResourceId const& id, int32_t size, void const* data, std::string_view mime_type) {
  ImageManager::Handle h(id);

  auto img = h.data;
  img->pixels = stbi_load_from_memory( (stbi_uc const *)data, size, &img->width, &img->height, &img->channels, kDefaultNumChannels); //
  img->channels = kDefaultNumChannels;

  if (nullptr == img->pixels) {
    LOG_WARNING( "Image Resource internal load failed for :", id.c_str());
    h.data.reset();
    h.data = nullptr;
  }
  LOG_DEBUG_INFO(__FUNCTION__, id.c_str());

  return h;
}

void ImageManager::setup_crossed_hdr(Image &img) {
  assert(img.width / 3 == img.height / 4);
  assert(img.width % 3 == img.height % 4);

  int32_t const w = img.width / 3;
  int32_t const h = img.height / 4;
  int32_t const face_size = w * h * img.channels;
  int32_t constexpr kCubeFaces = 6;

  float *data = (float*)STBI_MALLOC(kCubeFaces * face_size * sizeof(float));

  // Offsets of cubemap faces in img.pixels.
  std::array<int32_t[2], kCubeFaces> constexpr offsets{
    2, 1,          0, 1,
    1, 0,          1, 2,
    1, 1,          1, 4 
  };

  // Copy the first five face, line by line, top to bottom.
  int32_t const kLineWidth{ w * img.channels };
  size_t const kLineSize{ kLineWidth * sizeof(float) };
  for (int i = 0; i < kCubeFaces - 1; ++i) {
    int32_t const x = offsets[i][0] * w;
    int32_t const y = offsets[i][1] * h;
    int32_t const face_index = i * face_size;

    for (int j = 0; j < h; ++j) {
      int32_t const dst_index = face_index + j * w * img.channels;
      int32_t const src_index = ((y + j) * img.width + x) * img.channels;

      memcpy( data + dst_index, (float*)img.pixels + src_index, kLineSize);
    }
  }

  // Note : the last face (-Z) is reversed compared to others. There are many way
  // to solve the issue : custom skybox attributes via geometry stage, reorganizing
  // via compute shaders, but we use a simple one thread CPU transpose.

  int32_t const x = offsets[kCubeFaces-1][0] * w;
  int32_t const y = offsets[kCubeFaces-1][1] * h;
  int32_t const face_index = (kCubeFaces-1) * face_size;
  for (int j = 0; j < h; ++j) {
    int32_t const dst_index = face_index + j * w * img.channels;
    int32_t const src_index = ((y-j-1) * img.width + x) * img.channels;

    for (int k = 0; k < kLineWidth; k+=img.channels) {
      int32_t dst = dst_index + k;
      int32_t src = src_index + kLineWidth - k - img.channels;

      data[dst + 0] = ((float*)img.pixels)[src + 0];
      data[dst + 1] = ((float*)img.pixels)[src + 1];
      data[dst + 2] = ((float*)img.pixels)[src + 2];
      data[dst + 3] = ((float*)img.pixels)[src + 3]; //
    }
  }

  stbi_image_free(img.pixels);
  img.pixels = data;
  
  // Change image parameters.
  img.width  = w;
  img.height = h;
  img.depth  = kCubeFaces;
}

// ----------------------------------------------------------------------------