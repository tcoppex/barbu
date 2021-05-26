#ifndef BARBU_MEMORY_HASH_ID_H_
#define BARBU_MEMORY_HASH_ID_H_

#include <functional>
#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map>

// ----------------------------------------------------------------------------

//
// Hold an id (string and hash) to be used in a map.
//
// * We can't hash the path into a shared string map as we need a value to
// resolve hash collisions. A naive solution would be to store a few
// bytes of the full path, eg. a second hash of the basename - or a second hash
// using a different algorithm. 
// Storing two hashes : one being the check value of the second.
//
//  For now we keep the full pathname locally.
//
struct HashId {
  explicit
  HashId(std::string_view _s) 
    : path(_s)
    , h(std::hash<std::string_view>{}(_s)) 
  {}

  // implicit
  HashId(char const* _s) 
    : HashId(std::string_view(_s))
  {}

  // Return a unique HashId of format "basename (#)" using the is_taken functor
  // to test its availability.
  static
  HashId FindUnique(std::string_view basename, std::function<bool(HashId const&)> is_taken) {
    std::string id_string( basename );

    int index = 0;
    while (is_taken(HashId(id_string))) {
      std::stringstream ss;
      ss << basename << " (" << (++index) << ")";
      id_string = ss.str();
    }
    return HashId(id_string);
  }

  // (used to bypass a resource in a method parameter)
  HashId(std::nullptr_t) : path(""), h(0) {}

  // Check value when hash collided.
  inline bool operator ==(HashId const& rhs) const noexcept {
    return path == rhs.path;
  }

  inline operator std::string_view() const noexcept {
    return path;
  }

  inline operator size_t() const noexcept { 
    return h; 
  }
  
  inline std::string const& str() const noexcept {
    return path;
  }

  inline char const* c_str() const noexcept {
    return path.c_str();
  }

  std::string const path; //
  size_t const h;
};

// ----------------------------------------------------------------------------

// hash operator overload.
namespace std {

template <> struct hash<HashId> {
  inline size_t operator ()(HashId const& x) const {
    return x;
  }
};

} // namespace std

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_HASH_ID_H_