#include "memory/assets/texture.h"

#include <algorithm>
#include "core/graphics.h"

// ----------------------------------------------------------------------------

namespace {

void UseLinearInternalFormat(std::string const& filename, int32_t &internalFormat, bool bForce = false) {
  std::string fn(Logger::TrimFilename(filename));

  // Transform filename to lowercase to test matches.
  std::transform( fn.begin(), fn.end(), fn.begin(), ::tolower);
  
  // Find token for non linear texture.
  std::string const kLinearToken[]{
    //"bump", "normal", 
    "alpha", "mask",
  };
  for (auto const& s : kLinearToken) {
    if (fn.find(s) != fn.npos) {
      LOG_DEBUG_INFO(__FUNCTION__, ": Token", s, "was found in", fn);
      return;
    }
  }

  // Test extension.
  std::string ext = fn.substr(fn.find_last_of(".") + 1);  
  bool const is_internal = (ext == fn);
  std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower);
  
  if (bForce
  || is_internal
  || ("jpg" == ext) 
  || ("jpeg" == ext) 
  || ("bmp" == ext) 
  || ("png" == ext)) 
  {
    internalFormat = (internalFormat == GL_RGB8) ? GL_SRGB8 :
                     (internalFormat == GL_RGBA8) ? GL_SRGB8_ALPHA8 :
                     internalFormat
                     ;
  }
}

// Fill "format" and "type" based on internalFormat.
void GetTextureInfo(int32_t internalFormat, int32_t &format, int32_t &type) {
  switch (internalFormat) {
    // uint8
    case GL_R8:
      format = GL_RED;
      type = GL_UNSIGNED_BYTE;
    break;
    case GL_RG8:
      format = GL_RG;
      type = GL_UNSIGNED_BYTE;
    break;
    case GL_RGB8:
    case GL_SRGB8:
      format = GL_RGB;
      type = GL_UNSIGNED_BYTE;
    break;
    case GL_RGBA8:
    case GL_SRGB8_ALPHA8:
      format = GL_RGBA;
      type = GL_UNSIGNED_BYTE;
    break;

    // uint16
    case GL_R16:
      format = GL_RED;
      type = GL_UNSIGNED_SHORT;
    break;
    case GL_RG16:
      format = GL_RG;
      type = GL_UNSIGNED_SHORT;
    break;
    case GL_RGB16:
      format = GL_RGB;
      type = GL_UNSIGNED_SHORT;
    break;
    case GL_RGBA16:
      format = GL_RGBA;
      type = GL_UNSIGNED_SHORT;
    break;

    // float16
    case GL_R16F:
      format = GL_RED;
      type = GL_FLOAT;
    break;
    case GL_RG16F:
      format = GL_RG;
      type = GL_FLOAT;
    break;
    case GL_RGB16F:
      format = GL_RGB;
      type = GL_FLOAT;
    break;
    case GL_RGBA16F:
      format = GL_RGBA;
      type = GL_FLOAT;
    break;

    // float32
    case GL_R32F:
      format = GL_RED;
      type = GL_FLOAT;
    break;
    case GL_RG32F:
      format = GL_RG;
      type = GL_FLOAT;
    break;
    case GL_RGB32F:
      format = GL_RGB;
      type = GL_FLOAT;
    break;
    case GL_RGBA32F:
      format = GL_RGBA;
      type = GL_FLOAT;
    break;

    // [wip] Depth (should not be used)
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
      format = GL_RED;
      type = GL_FLOAT;
    break;

    default:
      LOG_FATAL_ERROR( "Internal format", internalFormat, "is not implemented." );
    break;
  };
}

} // namespace

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void Texture::allocate() {
  assert( params.target > 0 );
  if (!loaded()) {
    glCreateTextures(params.target, 1, &id);
  }
  CHECK_GX_ERROR();
}

void Texture::release() {
  if (loaded()) {
    glDeleteTextures( 1, &id);
    id = 0;
  }
  CHECK_GX_ERROR();
}

bool Texture::setup() {
  auto & resources = params.dependencies;
  int const nresources = static_cast<int>(resources.size());

  // [ should we keep a Resource_t object internally as well ? ]
  // Default creation parameters when no resources are linked.
  int32_t w = params.w;
  int32_t h = params.h;
  int32_t z = params.depth;
  void *pixels = params.pixels; //

  // Detect potential gamma-corrected format and stored them to sRGB [linear space]
  // (the pipeline should output gamma-corrected image).
  if (nresources > 0) {
    std::string const fn( resources[0].id );
    UseLinearInternalFormat( fn, params.internalFormat);
  }

  // Deduced format & type from internalFormat.
  int32_t format, type;
  GetTextureInfo(params.internalFormat, format, type);

  //------------------------------------------

  bool const is_2d = (GL_TEXTURE_2D == params.target);
  bool const is_3d = (GL_TEXTURE_3D == params.target) || (GL_TEXTURE_2D_ARRAY == params.target);

  // Target dependent texture setup.
  if (is_2d || is_3d) {
    bool bCreateStorage = false;

    // Retrieve the image resource.
    if (!resources.empty()) {
      auto const img = Resources::GetUpdated<Resource_t>( resources[0] ).data;
      if (!img) {
        return false; 
      }
      w = img->width;
      h = img->height;
      z = img->depth;
      pixels = img->pixels;

      // [fixme] Force format consistency.. 
      format = (img->channels == 4) ? GL_RGBA : 
               (img->channels == 3) ? GL_RGBA : //
               (img->channels == 2) ? GL_RG : 
                                      GL_RED;
    
      if (kImmutableResolution) {
        bCreateStorage = (resources[0].version <= 0);
      } else {
        bool const resolution_changed = (w != params.w) || (h != params.h);
        if (resolution_changed && (resources[0].version > 0)) {
          release();
          allocate();
        }
        bCreateStorage = resolution_changed;
      }
    } else {
      bCreateStorage = true;
    }
    
    // Clamp levels to maximum.
    params.levels = glm::min(params.levels, GetMaxMipLevel(w, h));  

    // [ somes cases might have been missed ]
    if (bCreateStorage) {
      is_2d ? glTextureStorage2D(id, params.levels, params.internalFormat, w, h) :
      is_3d ? glTextureStorage3D(id, params.levels, params.internalFormat, w, h, z)
            : [](){}();
    }
    
    if (pixels) {
      is_2d ? glTextureSubImage2D(id, 0, 0, 0, w, h, format, type, pixels) :
      is_3d ? glTextureSubImage3D(id, 0, 0, 0, 0, w, h, z, format, type, pixels)
            : [](){}();
    }
  }
  else if (GL_TEXTURE_CUBE_MAP == params.target) 
  {
    constexpr int kCubeFaces = 6;
    
    if ((nresources == kCubeFaces) || (nresources == 1)) {
      // Individual faces or crossed HDR image.

      for (int i = 0; i < nresources; ++i) {
        auto img = Resources::GetUpdated<Resource_t>( resources[i] ).data;
        if (nullptr == img) {
          return false;
        }

        w = img->width;
        h = img->height;
        z = img->depth;
        pixels = img->pixels;

        // Defines storage only on first pass. 
        // [should also check the resolution has not changed !]
        if ((0 == i) && (resources[0].version <= 0)) {
          glTextureStorage2D(id, params.levels, params.internalFormat, w, h); //
        }
        
        if (nresources == kCubeFaces) {
          // (multiple rgb8 images)
          glTextureSubImage3D(  id, 0,  0, 0, i, w, h, 1, format, type, pixels);

        } else if ((nresources == 1) && (z == kCubeFaces)) {
          // crossed HDR (array of rgb floating point image)
          int32_t const face_size = w * h * img->channels;

          for (int j = 0; j < z; ++j) {
            void *data = (float*)pixels + j * face_size;
            glTextureSubImage3D(  id, 0,  0, 0, j,  w, h, 1,  format, type, data);
          }
        }
      }
    } else if (nresources == 0) {
      // Empty cubemap.
      glTextureStorage2D(id, params.levels, params.internalFormat, w, h);
    } else {
      LOG_ERROR( "Cubemap format not implemented." );
    }
  }
  else
  {
    LOG_ERROR("Texture target", params.target, "was not implemented.");
    return false;
  }

  //------------------------------------------

  // Set a default internal sampler. 
  // glTextureParameteri( id, GL_TEXTURE_WRAP_S,      GL_CLAMP_TO_EDGE);
  // glTextureParameteri( id, GL_TEXTURE_WRAP_T,      GL_CLAMP_TO_EDGE);
  // glTextureParameteri( id, GL_TEXTURE_WRAP_R,      GL_CLAMP_TO_EDGE);
  // glTextureParameteri( id, GL_TEXTURE_MIN_FILTER,  GL_LINEAR);
  // glTextureParameteri( id, GL_TEXTURE_MAG_FILTER,  GL_LINEAR);  

  // Generate mipmaps when needed.
  if ((params.levels > 1) && pixels) {
    generate_mipmaps();
  }

  // Save resource dependent setup parameters.
  params.w = w;
  params.h = h;
  params.depth = z;

  // Empty pixels ptr if any.
  params.pixels = nullptr; //

  CHECK_GX_ERROR();

  return true;
}

void Texture::generate_mipmaps() {
  glGenerateTextureMipmap(id);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

TextureFactory::Handle TextureFactory::create2d(AssetId const& id, int levels, int internalFormat, ResourceId const& resource) {
  assert(levels >= 1);
  Parameters_t params;
  params.target         = GL_TEXTURE_2D;
  params.levels         = levels;
  params.internalFormat = internalFormat;
  params.dependencies.add_resource( (resource.h == 0) ? id : resource );
  return create(id, params);
}

TextureFactory::Handle TextureFactory::create2d(AssetId const& id, ResourceId const& resource) {
  return create2d(id, 4, GL_RGBA8, resource); //
}

TextureFactory::Handle TextureFactory::create2d(AssetId const& id, int levels, int internalFormat, int w, int h, void *pixels) {
  assert(levels >= 1);
  Parameters_t params;
  params.target         = GL_TEXTURE_2D;
  params.levels         = levels;
  params.internalFormat = internalFormat;
  params.w              = w;
  params.h              = h;
  params.pixels         = pixels;
  return create(id, params);
}

TextureFactory::Handle TextureFactory::create2dArray(AssetId const& id, int levels, int internalFormat, int w, int h, int d, void *pixels) {
  assert(levels >= 1);
  assert(w >= 1 && h >= 1 && d >= 1);
  Parameters_t params;
  params.target         = GL_TEXTURE_2D_ARRAY;
  params.levels         = levels;
  params.internalFormat = internalFormat;
  params.w              = w;
  params.h              = h;
  params.depth          = d;
  params.pixels         = pixels;
  return create(id, params);
}

TextureFactory::Handle TextureFactory::create3d(AssetId const& id, int levels, int internalFormat, int w, int h, int d, void *pixels) {
  assert(levels >= 1);
  assert(w >= 1 && h >= 1 && d >= 1);
  Parameters_t params;
  params.target         = GL_TEXTURE_3D;
  params.levels         = levels;
  params.internalFormat = internalFormat;
  params.w              = w;
  params.h              = h;
  params.depth          = d;
  params.pixels         = pixels;
  return create(id, params);
}

TextureFactory::Handle TextureFactory::createCubemap(AssetId const& id, int levels, int internalFormat, ResourceInfoList const& dependencies) {
  assert(levels >= 1);
  Parameters_t params;
  params.target         = GL_TEXTURE_CUBE_MAP;
  params.levels         = levels;
  params.internalFormat = internalFormat;
  params.dependencies   = dependencies;
  return create( id, params);
}

TextureFactory::Handle TextureFactory::createCubemap(AssetId const& id, ResourceInfoList const& dependencies) {
  int32_t const levels  = 1;
  int32_t const internalFormat = GL_RGBA8;
  return createCubemap(id, levels, internalFormat, dependencies);  
}

TextureFactory::Handle TextureFactory::createCubemap(AssetId const& id, int levels, int internalFormat, int w, int h, void *pixels) {
  //levels = (levels < 0) ? Texture::GetMaxMipLevel( w, h) : levels; //
  assert(levels >= 1);
  Parameters_t params;
  params.target         = GL_TEXTURE_CUBE_MAP;
  params.levels         = levels;
  params.internalFormat = internalFormat;
  params.w              = w;
  params.h              = h;
  params.pixels         = pixels;
  return create(id, params);
}

// ----------------------------------------------------------------------------

TextureFactory::Handle TextureFactory::createCubemapHDR(AssetId const& id, int levels, ResourceId const& resource) {
  assert(levels >= 1);
  Parameters_t params;
  params.target         = GL_TEXTURE_CUBE_MAP;
  params.levels         = levels;
  params.internalFormat = GL_RGBA16F; //
  params.dependencies.add_resource( (resource.h == 0) ? id : resource );
  return create(id, params);
}

// ----------------------------------------------------------------------------
