#ifndef BARBU_MEMORY_RESOURCES_SHADER_H_
#define BARBU_MEMORY_RESOURCES_SHADER_H_

#include "memory/resource_manager.h"

// ----------------------------------------------------------------------------
//
//  Note : Include shaders are not live-tracked for now, their root files should
//         be touched to be hot-reloaded.
//
//         An easy implementation would track the includers of each includee in
//         a separate hashmap of set of resourceId, and on reload, if the shader
//         is an include, reload back its parents.
//
// ----------------------------------------------------------------------------

enum ShaderType {
  kNone = -1,

  kVertex, 
  kTessControl, 
  kTessEval, 
  kGeometry, 
  kFragment, 
  kCompute,

  kNumShaderType
};

struct Shader : public Resource {
  ShaderType type;
  uint32_t id = 0;

  ~Shader() {
    release();
  }

  void release() final;

  bool loaded() const noexcept final {
    return id > 0;
  }

  // Return the API-specific target for the shader based on its type.
  int32_t target() const;
};

// ----------------------------------------------------------------------------

class ShaderManager : public ResourceManager<Shader> {
 public:
  // Maximum size per shader file with includes (32 Kb).
  static constexpr int32_t kMaxShaderBufferSize = 32 * 1024;

 public:
  ShaderManager() {
    buffer_ = new char[kMaxShaderBufferSize]{};
  }

  ~ShaderManager() {
    delete [] buffer_;
    buffer_ = nullptr;
  }

 private:
  Handle _load(ResourceId const& id) final;
  Handle _load_internal(ResourceId const& id, int32_t size, void const* data, std::string_view mime_type) final { return Handle(); }

  char *buffer_ = nullptr;
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_RESOURCES_SHADER_H_