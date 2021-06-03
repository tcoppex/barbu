#ifndef BARBU_MEMORY_ASSETS_PROGRAM_H_
#define BARBU_MEMORY_ASSETS_PROGRAM_H_

#include <array>
#include "memory/asset_factory.h"
#include "memory/resources/shader.h"
#include "core/graphics.h"

// ----------------------------------------------------------------------------

using ProgramParameters = AssetParameters;

// ----------------------------------------------------------------------------

struct Program : Asset<ProgramParameters, Shader> {
 public:
  Program(Parameters_t const& _params)
    : Asset(_params)
  {}
  
  ~Program() { release(); }

  bool loaded() const noexcept final {
    return id != 0u;
  }

  // Set a program uniform giving a location.
  template<typename T>
  void setUniform(int32_t loc, T const& value);

  // Set a program uniform given its name and debug info.
  template<typename T>
  void setUniform(std::string_view name, T const& value) {
    auto const loc = gx::UniformLocation(id, name.data());
    if (loc > -1) {
      setUniform<T>( loc, value);
    }
  }

 public:
  uint32_t id = 0u;

 private:
  void allocate() final;
  void release() final;
  bool setup() final;

  // ids of attached shaders.
  std::array<uint32_t, kNumShaderType> shaders_; 

  template<typename> friend class AssetFactory;
};

// ----------------------------------------------------------------------------

using ProgramHandle = AssetHandle<Program>;

// ----------------------------------------------------------------------------

class ProgramFactory : public AssetFactory<Program> {
 public:
  ~ProgramFactory() { release_all(); }

  Handle createFull(AssetId const& id, ResourceId const& vs, ResourceId const& tcs, ResourceId const& gs, ResourceId const& tes, ResourceId const& fs);
  Handle createTess(AssetId const& id, ResourceId const& vs, ResourceId const& tcs, ResourceId const& tes, ResourceId const& fs);
  Handle createGeo(AssetId const& id, ResourceId const& vs, ResourceId const& gs, ResourceId const& fs = nullptr);
  Handle createRender(AssetId const& id, ResourceId const& vs, ResourceId const& fs);
  Handle createCompute(AssetId const& id, ResourceId const& cs = nullptr);

 private:
  bool post_setup(AssetId const& assetId, ProgramFactory::Handle h) final;
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_ASSETS_PROGRAM_H_
