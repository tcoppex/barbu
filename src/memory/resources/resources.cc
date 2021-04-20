#include "memory/resources/resources.h"

// ----------------------------------------------------------------------------

#define DEFINE_MANAGER(name) \
  name##Manager Resources::s##name ; \
  \
  template<> \
  bool Resources::Has< name >( ResourceId const& id ) { \
    return s##name.has( id ); \
  } \
  \
  template<> \
  name##Manager::Handle Resources::LoadInternal< name >( ResourceId const& id, int32_t size, void const* data, std::string_view mime_type ) { \
    return s##name.load_internal( id, size, data, mime_type); \
  } \
  \
  template<> \
  name##Manager::Handle Resources::Get< name >( ResourceId const& id ) { \
    return s##name.get( id ); \
  } \
  \
  template<> \
  name##Manager::Handle Resources::GetUpdated< name >( ResourceInfo & info ) { \
    return s##name.get_updated( info ); \
  } \
  \
  template<> \
  ResourceId Resources::Add< name >( std::string const& basename, name const& resource ) { \
    return s##name.add( basename, resource ); \
  } \
  \
  template<> \
  bool Resources::CheckVersion< name >( ResourceInfo const& info ) { \
    return s##name.check_version( info ); \
  }

DEFINE_MANAGER(Image)
DEFINE_MANAGER(MeshData)
DEFINE_MANAGER(Shader)

// ----------------------------------------------------------------------------