#include "memory/assets/texture.h"

#include <algorithm>
#include "core/graphics.h"

// ----------------------------------------------------------------------------

namespace {

void UseLinearInternalFormat(std::string fn, int32_t &internalFormat, bool bForce = false) {
  // Transform filename to lowercase to test matches.
  std::transform( fn.begin(), fn.end(), fn.begin(), ::tolower);
  
  // Find token for non linear texture.
  std::string const kLinearToken[]{
    "bump", "normal"
  };
  for (auto const& s : kLinearToken) {
    if (fn.find(s) != fn.npos) {
      return;
    }
  }

  // Test extension.
  auto const ext = fn.substr(fn.find_last_of(".") + 1);  
  bool const is_internal = (ext == fn);
  
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

  // Target dependent texture setup.
  // [ ! only TEXTURE_2D and CUBE_MAP have been implemented ]
  if (GL_TEXTURE_2D == params.target) 
  {
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
    }
    bool const resolution_changed = (w != params.w) || (h != params.h);
    
    // [ somes cases might have been missed ]
    if (kImmutableResolution) {
      if (resources[0].version <= 0) {
        glTextureStorage2D(id, params.levels, params.internalFormat, w, h);
      }
    } else if (resolution_changed) {
      if (resources[0].version > 0) {
        release();
        allocate();
      }
      glTextureStorage2D(id, params.levels, params.internalFormat, w, h);
    }

    glTextureSubImage2D(id, 0, 0, 0, w, h, format, type, pixels);
  } 
  else if (GL_TEXTURE_CUBE_MAP == params.target) 
  {
    constexpr int kCubeFaces = 6;
    assert((nresources == kCubeFaces) || (nresources == 1));

    glBindTextureUnit(0, id);
    
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
        glTextureStorage2D(id, params.levels, params.internalFormat, w, h);
      }
      
      if (nresources == kCubeFaces) {
        // (multiple rgb8 images)
        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, w, h, format, type, pixels);
      } else if ((nresources == 1) && (z == kCubeFaces)) {
        // crossed HDR (array of rgb floating point image)
        int32_t const face_size = w * h * img->channels;
        for (int j = 0; j<z; ++j) {
          glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, 0, 0, w, h, format, type, (float*)pixels + j * face_size);
        }
      }
    }
    
    glBindTextureUnit(0, 0);
  }
  else
  {
    LOG_ERROR("Texture target", params.target, "was not implemented.");
    return false;
  }

  //------------------------------------------

  // Generate mipmaps when requested.
  if (params.levels > 1) {
    glGenerateTextureMipmap(id);
  }

  // Save resource dependent setup parameters.
  params.w = w;
  params.h = h;
  params.depth = z;

  // Empty pixels ptr if any.
  params.pixels = nullptr;

  CHECK_GX_ERROR();

  return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

TextureFactory::Handle TextureFactory::create2d(AssetId const& id, int levels, int internalFormat, ResourceId const& resource) {
  Parameters_t params;
  params.target         = GL_TEXTURE_2D;
  params.levels         = levels;
  params.internalFormat = internalFormat;
  params.dependencies.add_resource( (resource.h == 0) ? id : resource );
  return create(id, params);
}

TextureFactory::Handle TextureFactory::create2d(AssetId const& id, ResourceId const& resource) {
  int32_t const levels = 4;
  int32_t const internalFormat = GL_RGB8;
  return create2d(id, levels, internalFormat, resource);
}

TextureFactory::Handle TextureFactory::create2d(AssetId const& id, int levels, int internalFormat, int w, int h, void *pixels) {
  Parameters_t params;
  params.target         = GL_TEXTURE_2D;
  params.levels         = levels;
  params.internalFormat = internalFormat;
  params.w              = w;
  params.h              = h;
  params.pixels         = pixels;
  return create(id, params);
}

// ----------------------------------------------------------------------------

TextureFactory::Handle TextureFactory::createCubemap(AssetId const& id, int levels, int internalFormat, ResourceInfoList const& dependencies) {
  Parameters_t params;
  params.target         = GL_TEXTURE_CUBE_MAP;
  params.levels         = levels;
  params.internalFormat = internalFormat;
  params.dependencies   = dependencies;
  return create( id, params);
}

TextureFactory::Handle TextureFactory::createCubemap(AssetId const& id, ResourceInfoList const& dependencies) {
  int32_t const levels = 1;
  int32_t const internalFormat = GL_RGB8;
  return createCubemap(id, levels, internalFormat, dependencies);  
}

TextureFactory::Handle TextureFactory::createCubemapHDR(AssetId const& id, ResourceId const& resource) {
  Parameters_t params;
  params.target         = GL_TEXTURE_CUBE_MAP;
  params.levels         = 1;
  params.internalFormat = GL_RGB16F;
  params.dependencies.add_resource( (resource.h == 0) ? id : resource );
  return create(id, params);
}

// ----------------------------------------------------------------------------
