#include "memory/assets/program.h"

#include "glm/gtc/type_ptr.hpp"
#include "core/graphics.h"

// ----------------------------------------------------------------------------

template<typename T>
void Program::setUniform(int32_t loc, T const& value) {
  assert( false && "template not implemented" );
}

template<>
void Program::setUniform(int32_t loc, float const& value) {
  gx::SetUniform( id, loc, value);
}

template<>
void Program::setUniform(int32_t loc, int32_t const& value) {
  gx::SetUniform( id, loc, value);
}

template<>
void Program::setUniform(int32_t loc, uint32_t const& value) {
  gx::SetUniform( id, loc, value);
}

template<>
void Program::setUniform(int32_t loc, glm::vec2 const& value) {
  gx::SetUniform( id, loc, value);
}

template<>
void Program::setUniform(int32_t loc, glm::vec3 const& value) {
  gx::SetUniform( id, loc, value);
}

template<>
void Program::setUniform(int32_t loc, glm::vec4 const& value) {
  gx::SetUniform( id, loc, value);
}

template<>
void Program::setUniform(int32_t loc, glm::mat3 const& value) {
  gx::SetUniform( id, loc, value);
}

template<>
void Program::setUniform(int32_t loc, glm::mat4 const& value) {
  gx::SetUniform( id, loc, value);
}

void Program::allocate() {
  if (!loaded()) {
    id = glCreateProgram();
  }
  CHECK_GX_ERROR();
}

void Program::release() {
  if (loaded()) {
    glDeleteProgram(id);
    id = 0u;
  }
  CHECK_GX_ERROR();
}

bool Program::setup() {
  for (auto &info : params.dependencies) {

    // Do not update unmodified dependencies.
    if ( (info.version > ResourceInfo::kDefaultVersion) 
      && Resources::Has<Shader>(info.id) 
      && !Resources::CheckVersion<Shader>(info)) {
      continue;
    }

    auto h = Resources::GetUpdated<Shader>( info );
    if (!h.is_valid()) {
      return false;
    }
    auto shader = h.data;

    // When the shader has been updated we flag the previous one for deletion.
    if (info.version > ResourceInfo::kDefaultVersion+1) {
      glDeleteShader( shaders_[shader->type] );
      glDetachShader( id, shaders_[shader->type]);
    }
    shaders_[shader->type] = shader->id;

    glAttachShader( id, shader->id); 
  }
  CHECK_GX_ERROR();

  return true;
}

// ----------------------------------------------------------------------------

ProgramFactory::Handle ProgramFactory::createFull(AssetId const& id, ResourceId const& vs, ResourceId const& tcs, ResourceId const& gs, ResourceId const& tes, ResourceId const& fs) {
  return create( id, {vs, tcs, gs, tes, fs} );
}

ProgramFactory::Handle ProgramFactory::createTess(AssetId const& id, ResourceId const& vs, ResourceId const& tcs, ResourceId const& tes, ResourceId const& fs) {
  return create( id, {vs, tcs, tes, fs} );  
}

ProgramFactory::Handle ProgramFactory::createGeo(AssetId const& id, ResourceId const& vs, ResourceId const& gs, ResourceId const& fs) {
  return fs ? create( id, {vs, gs, fs}) : create( id, {vs, gs} );  
}

ProgramFactory::Handle ProgramFactory::createRender(AssetId const& id, ResourceId const& vs, ResourceId const& fs) {
  return create( id, {vs, fs} );
}

ProgramFactory::Handle ProgramFactory::createCompute(AssetId const& id, ResourceId const& cs) {
  return cs ? create( id, { cs } ) : create(id);
}

bool ProgramFactory::post_setup(AssetId const& assetId, ProgramFactory::Handle h) {
  assert( h->loaded() );
  gx::LinkProgram( h->id );
  return gx::CheckProgramStatus( h->id, assetId);
}

// ----------------------------------------------------------------------------