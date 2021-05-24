#ifndef BARBU_UTILS_MATHUTILS_H_
#define BARBU_UTILS_MATHUTILS_H_

#include <cmath>
#include <map>
#include "glm/glm.hpp"

// ----------------------------------------------------------------------------

// Vertex ordering operator used to reindex vertices from raw data.
namespace {

template<typename T>
constexpr bool almost_equal(T const& a, T const& b, T const eps = std::numeric_limits<T>::epsilon()) {
  //static_assert( std::is_floating_point<T>::value );
  T const distance = glm::abs(b - a);
  return (distance <= eps)
      || (distance < (std::numeric_limits<T>::min() * glm::abs(b + a)));
}

// ----------------------------------------------------------------------------

template<typename T>
struct vec2_ordering {
  static
  bool Less(const T& a, const T& b) {
    return (a.x < b.x)
        || (almost_equal(a.x, b.x) && (a.y < b.y));
  }

  bool operator() (const T& a, const T& b) const {
    return Less(a, b);
  }
};

template<typename T>
struct vec3_ordering {
  static
  bool Less(const T& a, const T& b) {
    const bool testX = almost_equal(a.x, b.x);
    return (a.x < b.x)
        || (testX && (a.y < b.y))
        || (testX && almost_equal(a.y, b.y) && (a.z < b.z));
  }

  bool operator() (const T& a, const T& b) const {
    return Less(a, b);
  }
};

template<typename T>
struct vec4_ordering {
  static
  bool Less(const T& a, const T& b) {
    const bool testX = almost_equal(a.x, b.x);
    const bool testY = almost_equal(a.y, b.y);
    return (a.x < b.x)
        || (testX && (a.y < b.y))
        || (testX && testY && (a.z < b.z))
        || (testX && testY && almost_equal(a.z, b.z) && (a.w < b.w));
  }

  bool operator() (const T& a, const T& b) const {
    return Less(a, b);
  }
};

// ----------------------------------------------------------------------------

template<typename K, typename V>
using MapVec2 = std::map<K, V, vec2_ordering<K>>;

template<typename K, typename V>
using MapVec3 = std::map<K, V, vec3_ordering<K>>;

template<typename K, typename V>
using MapVec4 = std::map<K, V, vec4_ordering<K>>;

} // namespace

// ----------------------------------------------------------------------------

#endif // BARBU_UTILS_MATHUTILS_H_