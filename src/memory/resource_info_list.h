#ifndef BARBU_MEMORY_RESOURCES_RESOURCE_INFO_LIST_H_
#define BARBU_MEMORY_RESOURCES_RESOURCE_INFO_LIST_H_

#include <string_view>
#include "memory/hash_id.h"
#include "memory/null_vector.h"

// ----------------------------------------------------------------------------

using ResourceId       = HashId;
using ResourceFlag     = uint32_t;
using ResourceVersion  = int32_t;

// ----------------------------------------------------------------------------

//
//  Keeps track of resource dependencies for an asset.
//
struct ResourceInfo {
  static constexpr ResourceFlag    kDefaultFlag    = 0u;  // no flags
  static constexpr ResourceVersion kDefaultVersion = -1;  // no version

  ResourceInfo(ResourceId const& _id)
    : id(_id)
    , flag(kDefaultFlag)
    , version(kDefaultVersion)
  {}

  ResourceInfo(ResourceId const& _id, ResourceFlag _flag, ResourceVersion _version)
    : id(_id)
    , flag(_flag)
    , version(_version)
  {}

  ResourceId      id;           //< Access-token to the resource.
  ResourceFlag    flag;         //< Complementary load flags.       // [unused]
  ResourceVersion version;      //< Local version of the resource.
};

// ----------------------------------------------------------------------------

class ResourceInfoList : public std::null_vector<ResourceInfo> {
 public:  
  ResourceInfoList() = default;
  ResourceInfoList(ResourceInfoList const&) = default;

  ResourceInfoList(std::nullptr_t) {
    set_to_nullptr();
  }

  ResourceInfoList(std::initializer_list<std::string_view> const& _init_list) {
    add_resources(_init_list);
  }

  template<typename Container>
  ResourceInfoList(Container const& _list) {
    add_resources(_list);
  }

  ResourceInfoList& operator =(std::nullptr_t) noexcept {
    set_to_nullptr();
    return *this;
  }

  ResourceInfoList& operator =(ResourceInfoList const& _rh) noexcept {
    set_to_nullptr();
    for (auto const& x : _rh) {
      push_back(x);
    }
    
    return *this;
  }

  void add_resource(ResourceId const& _id) {
    push_back( ResourceInfo(_id) );
  }

  void add_resource(std::string_view _id) {
    add_resource(ResourceId(_id));
  }

  void add_resources(std::initializer_list<std::string_view> const& _init_list) {
    for (auto &id : _init_list) {
      add_resource(id);
    }
  }

  template<typename Container>
  void add_resources(Container const& _list) {
    insert( end(), _list.cbegin(), _list.cend());
  }
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_RESOURCES_RESOURCE_INFO_LIST_H_
