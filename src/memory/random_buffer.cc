#include "memory/random_buffer.h"
#include "glm/vec4.hpp"

// ----------------------------------------------------------------------------

// (todo : use bindless instead)

void RandomBuffer::init(int const nelements) {
  num_elements_ = nelements;

  glGenBuffers(1u, &gl_buffer_id_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl_buffer_id_);

  size_t const bytesize = num_elements_ * sizeof(float);
  glBufferStorage(GL_SHADER_STORAGE_BUFFER, bytesize, nullptr, GL_MAP_WRITE_BIT);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0u);

  CHECK_GX_ERROR();
}

void RandomBuffer::deinit() {
  glDeleteBuffers(1u, &gl_buffer_id_);
}

void RandomBuffer::generate_values() {
  std::uniform_real_distribution<float> distrib(min_value_, max_value_);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl_buffer_id_);

  float *buffer = reinterpret_cast<float*>(glMapBufferRange(
    GL_SHADER_STORAGE_BUFFER, 0u, num_elements_ * sizeof(float),
    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT
  ));

  for (int i=0; i<num_elements_; ++i) {
    buffer[i] = distrib(mt_);
  }

  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0u);

  CHECK_GX_ERROR();
}

void RandomBuffer::bind(int binding) {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, gl_buffer_id_);
}

void RandomBuffer::unbind(int binding) {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, 0u);
}

// ----------------------------------------------------------------------------