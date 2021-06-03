#include "memory/random_buffer.h"
#include "glm/vec4.hpp"

// ----------------------------------------------------------------------------

// (todo : use bindless instead)

void RandomBuffer::init(int const nelements) {
  num_elements_ = nelements;
  size_t const bytesize = num_elements_ * sizeof(float);

  glCreateBuffers(1u, &gl_buffer_id_);
  glNamedBufferStorage( gl_buffer_id_, bytesize, nullptr, GL_MAP_WRITE_BIT);

  CHECK_GX_ERROR();
}

void RandomBuffer::deinit() {
  glDeleteBuffers(1u, &gl_buffer_id_);
}

void RandomBuffer::generate_values() {
  std::uniform_real_distribution<float> distrib(min_value_, max_value_);

  float *buffer = reinterpret_cast<float*>(glMapNamedBufferRange( 
    gl_buffer_id_, 0u, num_elements_ * sizeof(float),
    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT
  ));

  for (int i=0; i<num_elements_; ++i) {
    buffer[i] = distrib(mt_);
  }
  
  glUnmapNamedBuffer( gl_buffer_id_ );
  CHECK_GX_ERROR();
}

void RandomBuffer::bind(int binding) {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, gl_buffer_id_);
}

void RandomBuffer::unbind(int binding) {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, 0u);
}

// ----------------------------------------------------------------------------