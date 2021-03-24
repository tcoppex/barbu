#ifndef BARBU_MEMORY_NULL_VECTOR_H_
#define BARBU_MEMORY_NULL_VECTOR_H_

#include <memory>
#include <vector>

// ----------------------------------------------------------------------------
// This is a customization of std to allow vector set to nullptr. 
// ----------------------------------------------------------------------------

namespace std {

template<class T>
class null_vector : public std::vector<T> {
 public:
  null_vector() = default;

  null_vector(nullptr_t) {
    set_to_nullptr();
  }

  null_vector& operator= (nullptr_t) noexcept {
    set_to_nullptr();
    return *this;
  }

 protected:
  void set_to_nullptr() {
    std::vector<T>::clear();
    std::vector<T>::shrink_to_fit();
  }
};

template<class T>
bool operator== (const null_vector<T>& lhs, nullptr_t) noexcept { return lhs.capacity() == 0; }

template<class T>
bool operator== (nullptr_t, const null_vector<T>& rhs) noexcept { return 0 == rhs.capacity(); }

template<class T>
bool operator!= (const null_vector<T>& lhs, nullptr_t) noexcept { return lhs.capacity() > 0; }

template<class T>
bool operator!= (nullptr_t, const null_vector<T>& rhs) noexcept { return 0 < rhs.capacity(); }

} // namespace std

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_NULL_VECTOR_H_