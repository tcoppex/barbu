#ifndef BARBU_MEMORY_ASSETS_ASSET_FACTORY_H_
#define BARBU_MEMORY_ASSETS_ASSET_FACTORY_H_

#include <cassert>
#include <set>
#include <type_traits>

#include "core/logger.h"
#include "memory/resources/resources.h"
#include "memory/hash_id.h"

// ----------------------------------------------------------------------------
//
//    An Asset represent a transformed composite of resources used internally
//    by the application. It is strongly linked to a unique ResourceType
//    (ie. a texture is always made of image(s)).
//
//    An asset is built by its factory that use an AssetId and a AssetParameters
//    object to generate its internal data. An AssetParameters contains a list
//    of dependency resources [0 to n] needed to be loaded to create the asset.
//
//    The dependency list is stored in a ResourceInfoList consisting of a ResourceInfo
//    object. Each ResourceInfo hold the current resource version used by the
//    asset. When the factory calls update versionning is enabled and if the asset
//    use the versionning compatible ResoureceFactory::get_update method, the
//    AssetFactory will try to regenerate the asset every time the version changes. 
//
//  [ notes ]
//
//  * AssetId should probably *not* be user defined but computed by the AssetParameters
//    or dependencies-related when not null, and string defined when created from scratch.
//
//  * Not sure if the asset's internal allocation & setup should be in Asset or
//    in AssetFactory (right now in Asset and only accessible by the factory).
//
//  * Might be interesting to merge Asset and the factory - as static methods -
//    in a future version.
//
// ----------------------------------------------------------------------------

using AssetId = HashId;
template<typename T> class AssetFactory;

// ----------------------------------------------------------------------------

// - AssetParameters -

struct AssetParameters {
  AssetParameters() = default;
  
  AssetParameters(std::initializer_list<std::string_view> const& il) 
    : dependencies(il)
  {}
  
  virtual ~AssetParameters() {}
  ResourceInfoList dependencies;
};

// ----------------------------------------------------------------------------

// - Asset -

template<typename TParams, typename TResource>
class Asset {
  static_assert(
    std::is_convertible<TParams, AssetParameters>::value, 
    "Template parameter must derive from AssetParameters."
  );
 
 public: 
   using Parameters_t = TParams;
   using Resource_t   = TResource;

   TParams params; //

 public:
  Asset() = delete;
  
  Asset(TParams const& _params) 
    : params(_params)
  {}

  virtual ~Asset() {}

  // [change name ?]
  // Return true when the asset was correctly initialized, false otherwhise.
  virtual bool loaded() const noexcept = 0;

  // Return the specified dependency resource if it exists.
  ResourceHandle<TResource> get_resource(int index = 0) const {
    return (index < static_cast<int>(params.dependencies.size())) ? 
      Resources::Get<TResource>( params.dependencies[index].id ) : ResourceHandle<TResource>(); 
  }

 private:
  virtual void allocate() = 0;
  virtual void release() = 0;
  virtual bool setup() = 0;

  // !important : the derived class should always redeclare this.
  template<typename> friend class AssetFactory;
};

// ----------------------------------------------------------------------------

// - AssetHandle -

template<typename TAsset>
using AssetHandle = std::shared_ptr<TAsset>;

// ----------------------------------------------------------------------------

// - AssetFactory -

template<typename TAsset>
class AssetFactory {
 public:
  using Handle       = AssetHandle<TAsset>;
  using Parameters_t = typename TAsset::Parameters_t;
  using Resource_t   = typename TAsset::Resource_t;

 public:
  // When set to true, release methods will clear the asset entry by default.
  static constexpr bool kReleaseWipeOutDefault = false;

 public:
  AssetFactory() = default;

  virtual ~AssetFactory() {}

  // Create an asset from its parameters.
  Handle create(AssetId const& id, Parameters_t const& params) {
    assert( !id.path.empty() );

    // [ might be problematic, as parameters might be different ]
    if (has(id)) {
      return assets_.at(id);
    }

    // Create the handle.
    Handle h = std::make_shared<TAsset>(params);
    if (!h) {
      LOG_ERROR( "Could not create the asset \"", id.c_str(), "\".");
      return nullptr;
    }

    // Allocate internal memory.
    h->allocate();

    // Initialize.
    if (!setup(id, h)) {
      LOG_ERROR( "Could not initialize the asset \"", id.c_str(), "\".");
      return nullptr;
    }

    // Register the handle in the hashmap.
    assets_[id] = h;

    return h;
  }

  // Create the asset associate to its resource, with default setting.
  // This version will automatically bind a resource of the same id.
  Handle create(AssetId const& id) {
    Parameters_t params;
    params.dependencies.add_resource(id);
    return create( id, params );
  }

  // Release memory for the specified asset.
  // If bWipeOut is true clear the internal reference as well.
  void release(AssetId const& id, bool bWipeOut = kReleaseWipeOutDefault) {
    if (has(id)) {
      assets_.at(id)->release();
      if (bWipeOut) {
        assets_.erase(id);
      }
    }
  }

  // Release internal data associated with all assets references,
  // If bWipeOut is true clear the whole structure as well.
  void release_all(bool bWipeOut = kReleaseWipeOutDefault) {
    for (auto &tuple : assets_) {
      release(tuple.first);
    }
    if (bWipeOut) {
      assets_.clear();
    }
  }

  // Return true if the specified asset is in memory.
  inline bool has(AssetId const& id) const noexcept { 
    return assets_.find(id) != assets_.end();
  }

  // Return the specificed asset if present in memory.
  inline Handle get(AssetId const& id) const noexcept {
    return has(id) ? assets_.at(id) : nullptr;
  }

  // Wrapper around the asset setup, to be able to call a post_setup factory method.
  bool setup(AssetId const& id, Handle h) {
    return h->setup() && post_setup(id, h);
  }

  // Called after a successful setup. Optionnal.
  virtual bool post_setup(AssetId const& id, Handle h) { 
    return true; 
  }

  // Check if assets dependencies has been modified and update them accordingly.
  void update() {
    for (auto& tuple : assets_) {
      auto const id = tuple.first;
      auto handle   = tuple.second;
      for (auto & dep : handle->params.dependencies) {
        if (Resources::CheckVersion<Resource_t>(dep)) {
          setup(id, handle);
          break;
        }
      }
    }
  }

 private:
  using AssetHashmap_t = std::unordered_map< AssetId, Handle >;

  AssetHashmap_t assets_;

 private:
  // Disallow copy & move constructor.
  AssetFactory(AssetFactory const&) = delete;
  AssetFactory(AssetFactory &&) = delete;
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_ASSETS_ASSET_FACTORY_H_