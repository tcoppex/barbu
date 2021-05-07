#ifndef BARBU_UTILS_MATHUTILS_H_
#define BARBU_UTILS_MATHUTILS_H_

#include <cmath>
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

// Order comparison between two vec3.
template<typename T>
struct vec3_ordering {
  static
  bool Less(const T& a, const T& b) {
    return (a.x < b.x)
        || (almost_equal(a.x, b.x) && (a.y < b.y))
        || (almost_equal(a.x, b.x) && almost_equal(a.y, b.y) && (a.z < b.z));
  }

  bool operator() (const T& a, const T& b) const {
    return Less(a, b);
  }
};

} // namespace

// ----------------------------------------------------------------------------

#endif // BARBU_UTILS_MATHUTILS_H_