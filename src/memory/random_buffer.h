#ifndef BARBU_MEMORY_RANDOM_BUFFER_H_
#define BARBU_MEMORY_RANDOM_BUFFER_H_

#include <random>
#include "core/graphics.h"

// ----------------------------------------------------------------------------

// Holds a buffer of random floating point values on the device.
class RandomBuffer {
 public:
  RandomBuffer() :
    num_elements_(0u),
    gl_buffer_id_(0u),
    mt_(random_device_()),
    min_value_(0.0f),
    max_value_(1.0f)
  {}

  void init(int const nelements);
  void deinit();

  void generate_values();

  void bind(int binding);
  void unbind(int binding);

 private:
  int num_elements_;
  GLuint gl_buffer_id_;
  std::random_device random_device_;
  std::mt19937 mt_;
  float min_value_;
  float max_value_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_MEMORY_RANDOM_BUFFER_H_
