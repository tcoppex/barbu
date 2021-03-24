#ifndef BARBU_UTILS_SINGLETON_H_
#define BARBU_UTILS_SINGLETON_H_

#include <cassert>

// ----------------------------------------------------------------------------

template <class T>
class Singleton {
 public:
  static
  void Initialize() {
    assert(sInstance == nullptr);
    sInstance = new T;
  }

  static
  void Deinitialize() {
    if (sInstance != nullptr) {
      delete sInstance;
      sInstance = nullptr;
    }
  }

  static 
  T& Get() {
    assert(sInstance != nullptr);
    return *sInstance;
  }

 protected:
  Singleton() {}
  virtual ~Singleton() {}

 private:
  static T* sInstance;

 private:
  Singleton(Singleton const&) = delete;
  Singleton(Singleton&&) = delete;
};

template<class T> T* Singleton<T>::sInstance = nullptr;

// How to use :
// class Logger : public Singleton<Logger> {
//   friend class Singleton<Logger>;
//   // [class specific code]
// };

// ----------------------------------------------------------------------------

#endif  // BARBU_UTILS_SINGLETON_H_
