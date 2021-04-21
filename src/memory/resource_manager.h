#ifndef BARBU_MEMORY_RESOURCES_RESOURCE_MANAGER_H_
#define BARBU_MEMORY_RESOURCES_RESOURCE_MANAGER_H_

#include <cassert>
#include <memory>         // shared_ptr
#include <string>         // substr, find_last_of
#include <string_view>
#include <unordered_map>
#include <vector>
#include <thread>

// filesystem, used for versioning.
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/optional>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#pragma GCC error "std::filesystem is required."
//#pragma message( "std::filesystem is required." )
#endif

#include "core/logger.h"
#include "memory/resource_info_list.h"

// ----------------------------------------------------------------------------
//
//    A Resource represent an external raw data to be watched or loaded by the 
//    application. Resources are used to create Assets.
//
//    A Resource is loaded by its manager that use a ResourceId to locate its
//    data. ResourceId are most often the path to the external resource.
//
//    Internally resources have version attached to them, when the user modify
//    the external data (eg. by changing it in another program), the manager
//    will try to reupload it and modify its version accordingly.
//
//    When a resource is released its internal data are erased but its stats are
//    kept. All resources are released after a few frames.
//
//  [ notes ]
//
//  * ResourceHandle would probably be changed to be similar to AssetHandle, ie.
//    aliasing the asset shared_ptr rather than holding it.
//
//-----------------------------------------------------------------------------

//
// Abstract view of a resource, used to handle the specialized release of data.
//
class Resource {
 public:
  // [Temporary helper, to be externalized]
  static std::string TrimFilename(std::string const& filename) {
    return filename.substr(filename.find_last_of("/\\") + 1);
  }

 protected:
  virtual ~Resource() {}

  // Release the internal memory of the resource.
  virtual void release() = 0;

  // Return true if the resource is in memory.
  virtual bool loaded() const noexcept = 0;
};

// ----------------------------------------------------------------------------

//
// Hold the shared_ptr and the "ReadOnly" name for the resource.
// This is the data structure returned to the users.
//
template<typename TResource>
struct ResourceHandle {
  ResourceHandle() = default;

  ResourceHandle(ResourceId const& _id)
    : name(Resource::TrimFilename(_id.path))
    , data(std::make_shared<TResource>()) //
  {}

  // Clear the underlying resource when we were the last reference.
  ~ResourceHandle() {
    // [ Safeguard to be sure the data are deleted ]
    if (1 == data.use_count()) {
      data->release();
    }
  }
  
  // Return true if the resource was correctly loaded in memory.
  bool is_valid() const {
    return data && data->loaded();
  }

  std::string name; //
  std::shared_ptr<TResource> data;
};

// ----------------------------------------------------------------------------

//
// A ResourceManager handles the loading and lifetime of external datas to be 
// processed into Assets. 
//
// Most of the time this data are load from memory, but they can also be created directly.
//
// Resources are identified by a 'ResourceId' which must be the path of their
// external data (when it applies). When 'update' is called the manager checks
// if the data has been modified and load it back as necessary, updating its
// internal version.
//
// A user will call 'get' or 'load' to retrieve a resource and 'get_updated'
// to check if an already retrieved resource has been modified.
//
template<typename TResource>
class ResourceManager {
 private:
  static constexpr int32_t kLastWriteSpanMilliseconds = 250;

 public:
  using Handle = ResourceHandle<TResource>;

  ResourceManager() = default;

  virtual ~ResourceManager() {
    release_all();
  }

  // Release resources internal memory but keeps filestat info to track changes.
  inline void release_all() noexcept {
    resources_.clear();
  }

  // Check resources that has been modified.
  void update() {
    for (auto tuple : stats_) {
      auto const id   = tuple.first;
      auto const stat = tuple.second;
      auto sys_lw = sys_last_write(id);

      // ----------------
      auto has_time_elapsed = [](fs::file_time_type timemark, int32_t max_elapsed) -> bool {
        auto const now = fs::file_time_type::clock::now();
        auto const elapsed = now - timemark;
        auto const t = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        return t > max_elapsed;
      };
      // ----------------

      if (sys_lw != stat.last_write) {
        if (sys_lw == kDeletedLastWrite) {
          // (the file is not accessible anymore)
          update_stat(id);
          LOG_WARNING( "[VERSIONING] \"", id.path, "\" : file not found.");
        } else {

          // Check the file is not currently being saved.
          for (sys_lw = sys_last_write(id); !has_time_elapsed(sys_lw, kLastWriteSpanMilliseconds); sys_lw = sys_last_write(id)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kLastWriteSpanMilliseconds));
          }

          if (load(id).is_valid()) {
            // (the file has been modified)
            LOG_INFO( "[VERSIONING]", Resource::TrimFilename(id.path), ": v", version(id));
          }
        }
      }
    }
  }

  // Return true if the resource is on memory.
  inline bool has(ResourceId const& id) const noexcept {
    return (stats_.find(id) != stats_.end())              // has resource in memory
        || (resources_.find(id) != resources_.end())      // or just a stat  [ side effects ? ]
        ;
  }

  // Load a resource in memory and return an handle to it.
  inline Handle load(ResourceId const& id) {
    auto h = _load(id);
    if (h.is_valid()) {
      resources_[id] = h;
    
      // Update internal version.
      if (has(id)) {
        update_stat(id);
      }
    }
    return h;
  }

  Handle load_internal(ResourceId const& id, int32_t size, void const* data, std::string_view mime_type) {
    auto h = _load_internal(id, size, data, mime_type);
    if (h.is_valid()) {
      resources_[id] = h;
      
      // [check if the internal version works as intended]
      if (has(id)) {
        update_stat(id);
      }
    }
    return h;
  }

  // Retrieve the requested resource, or try reloading it when necessary.
  inline Handle get(ResourceId const& id) {
    auto const tuple = resources_.find(id);
    if (tuple == resources_.end()) {        // xxx
      return load(id);
    } else if (!(tuple->second).data->loaded()) {
      return load(id);
    }
    return tuple->second;
  }

  // Constant retrieval of a resource, without loading.
  inline Handle const get(ResourceId const& id) const {
    assert(has(id) && resources_[id].data->loaded());
    return resources_[id];
  }


  // Add a resource with id "basename (#)", " (#)" being a numbered suffix
  // when the id is already used.
  inline ResourceId add(std::string_view basename, TResource const& resource) {
    // (find unique id)
    ResourceId id( ResourceId::FindUnique(basename, 
      [this](ResourceId const& _id) { 
        return has(_id); 
      }) 
    );
    
    Handle h( id );
    *(h.data) = resource; //
    resources_[id] = h;
    update_stat(id);
    
    return id;
  }

  // Release access for the specified resource. [rename destroy ?]
  inline void release(ResourceId const& id) {
    resources_.erase(id);
  }

  // ------------------------------
  // VERSIONING.
  // ------------------------------

  // Special time used to indicated a file has been deleted, or does not exists
  // (eg. the data was internally created). Used to avoid logging deleted files ad-aeternum.
  static constexpr fs::file_time_type kDeletedLastWrite = fs::file_time_type::min();
  
  // Return the system's last write time for the given resource. 
  fs::file_time_type sys_last_write(ResourceId const& id) const noexcept {
    fs::path const filename(id.path);
    std::error_code ec;
    auto const lwt = fs::last_write_time(filename, ec);

    // When the file was deleted, we return the minimal time as indicator.
    if (ec && (last_write(id) > kDeletedLastWrite)) {
      return kDeletedLastWrite;
    }
    return lwt;
  }
  
  // Return the internal last write time for the file.
  fs::file_time_type last_write(ResourceId const& id) const noexcept {
    auto const& tuple = stats_.find(id);
    return (tuple != stats_.end()) ? tuple->second.last_write : kDeletedLastWrite;
  }

  // Return the internal version of a given resource.
  ResourceVersion version(ResourceId const& id) const noexcept {
    auto const& tuple = stats_.find(id);
    return (tuple != stats_.end()) ? tuple->second.version : ResourceInfo::kDefaultVersion;
  }

  // Return true when the resource has been updated, false otherwhise.
  bool check_version(ResourceInfo const& info) const noexcept {
    return has(info.id) 
        && (info.version > ResourceInfo::kDefaultVersion)
        && (info.version < version(info.id))
        ;
  }

  // Retrieve a resource based on a resource info and update its internal version
  // as needed. 
  // Use this version of 'get' to have hot-reload / versioning.
  inline Handle get_updated(ResourceInfo &info) {
    Handle h = get(info.id);
    info.version = version(info.id);
    return h;
  }

  //inline Handle get(ResourceId const& id, ResourceVersion &version);

  // Update the internal stat of a given resource.
  void update_stat(ResourceId const& id) {
    auto const& tuple = stats_.find(id);
    auto const last_write = sys_last_write(id);

    if (tuple == stats_.end()) {
      // (unwritten)
      stats_[id] = FileStat( last_write, 0);      
    } else if (tuple->second.last_write != last_write) {
      // (modified)
      auto const new_version = tuple->second.version + int((last_write != kDeletedLastWrite));
      stats_[id] = FileStat( last_write, new_version);
    }
  }


 private:
  // System file's informations used by versioning.
  struct FileStat {
    FileStat() = default;
    FileStat(fs::file_time_type const& _last_write, ResourceVersion _version)
     : last_write(_last_write)
     , version(_version)
    {}

    fs::file_time_type last_write;
    ResourceVersion    version;
  };

  using StatHashmap_t     = std::unordered_map< ResourceId, FileStat >;
  using ResourceHashmap_t = std::unordered_map< ResourceId, Handle >;

  // Specialized loader for the resource.
  virtual Handle _load(ResourceId const& id) = 0;
  virtual Handle _load_internal(ResourceId const& id, int32_t size, void const* data, std::string_view mime_type) = 0;

  StatHashmap_t stats_;
  ResourceHashmap_t resources_;

 private:
  ResourceManager(ResourceManager const&) = delete;
  ResourceManager(ResourceManager&&) = delete;
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_RESOURCES_RESOURCE_MANAGER_H_
