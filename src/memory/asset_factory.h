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
//  * An asset should probably keeps its id internally, to ease a lot of things.
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

  // When set to true, release methods will clear the asset entry by default.
  static constexpr bool kReleaseWipeOutDefault = false;

  AssetFactory() = default;
  
  virtual ~AssetFactory() = default;

  // Create an asset from its parameters.
  Handle create(AssetId const& _id, Parameters_t const& _params) {
    assert( !_id.path.empty() );

    // [ might be problematic, as parameters might be different ]
    if (has(_id)) {
      return assets_.at(_id);
    }

    // Create the handle.
    Handle h = std::make_shared<TAsset>(_params);
    if (!h) {
      LOG_ERROR( "Could not create the asset \"", _id.c_str(), "\".");
      return nullptr;
    }

    // Allocate internal memory.
    h->allocate();

    // Initialize.
    if (!setup(_id, h)) {
      LOG_ERROR( "Could not initialize the asset \"", _id.c_str(), "\".");
      return nullptr;
    }

    // Register the handle in the hashmap.
    assets_[_id] = h;

    return h;
  }

  // Create the asset associate to its resource, with default setting.
  // This version will automatically bind a resource of the same id.
  Handle create(AssetId const& _id) {
    Parameters_t params;
    params.dependencies.add_resource(_id);
    return create( _id, params );
  }

  // Release memory for the specified asset.
  // If bWipeOut is true clear the internal reference as well.
  void release(AssetId const& _id, bool _bWipeOut = kReleaseWipeOutDefault) {
    if (has(_id)) {
      assets_.at(_id)->release();
      if (_bWipeOut) {
        assets_.erase(_id);
      }
    }
  }

  // Release internal data associated with all assets references,
  // If bWipeOut is true clear the whole structure as well.
  void release_all(bool _bWipeOut = kReleaseWipeOutDefault) {
    for (auto &tuple : assets_) {
      release(tuple.first);
    }
    if (_bWipeOut) {
      assets_.clear();
    }
  }

  // Return true if the specified asset is in memory.
  inline bool has(AssetId const& _id) const noexcept { 
    return assets_.find(_id) != assets_.end();
  }

  AssetId findUniqueID(std::string_view _basename) {
    return AssetId::FindUnique( _basename, [this](AssetId const& _id) { return has(_id); });
  }

  // Return the specificed asset if present in memory.
  inline Handle get(AssetId const& _id) const noexcept {
    return has(_id) ? assets_.at(_id) : nullptr;
  }

  // Wrapper around the asset setup, to be able to call a post_setup factory method.
  bool setup(AssetId const& _id, Handle _h) {
    return _h->setup() && post_setup(_id, _h);
  }

  // Called after a successful setup. Optionnal.
  virtual bool post_setup(AssetId const& _id, Handle _h) { 
    return true; 
  }

  // Check if assets dependencies has been modified and update them accordingly.
  void update() {
    std::vector<AssetId> release_ids;

    for (auto& tuple : assets_) {
      auto const id = tuple.first;
      auto handle   = tuple.second;
      
      // (The asset could be destroyed once its has a unique reference left.)
      if ((handle.use_count() - 1) == 1) {
        release_ids.push_back(id);
      }

      // Update dependencies.
      for (auto & dep : handle->params.dependencies) {
        if (Resources::CheckVersion<Resource_t>(dep)) {
          setup(id, handle);
          break;
        }
      }
    }

    // Some code dont keep references, some assets are internals and dont have external references (eg. Default Material).
    // So we avoid clearing assets with a unique reference will for now. 
    if (bReleaseUniqueAssets_) {
      for (auto id : release_ids) {
        LOG_DEBUG_INFO("* Releasing asset", Logger::TrimFilename(id.str()));
        release(id);
      }
    }
  }

 protected:
  using AssetHashmap_t = std::unordered_map< AssetId, Handle >;
  AssetHashmap_t assets_;

  // When set to true release unique assets every update (the one only belonging to the factory.)
  bool bReleaseUniqueAssets_ = true;

 private:
  // Disallow copy & move constructor.
  AssetFactory(AssetFactory const&) = delete;
  AssetFactory(AssetFactory &&) = delete;
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_ASSETS_ASSET_FACTORY_H_