#include "memory/pingpong_buffer.h"
#include <cassert>

// ----------------------------------------------------------------------------

void PingPongBuffer::setup(int const nelems, int const base_binding, int const nattribs, bool const bUseSOALayout) {
  assert(((nelems > 0) || (base_binding >= 0) || (nattribs > 0)) && "Invalid arguments");
  
  if (nelems_ > 0) {
    destroy();
  }

  nelems_ = nelems;
  base_binding_ = base_binding;
  nattribs_ = nattribs;
  attrib_buffer_bytesize_ = nelems_ * kAttribBytesize;
  storage_buffer_bytesize_ = nattribs * attrib_buffer_bytesize_;
  bUseSOALayout_ = bUseSOALayout;

  glCreateBuffers(kNumBuffers, device_storage_ids_.data());
  for (auto buffer_id : device_storage_ids_) {
    glNamedBufferStorage( buffer_id, storage_buffer_bytesize_, nullptr,  GL_DYNAMIC_STORAGE_BIT); 
  }

  CHECK_GX_ERROR();
}

void PingPongBuffer::destroy() {
  if (nelems_ > 0) {
    glDeleteBuffers(kNumBuffers, device_storage_ids_.data());
    nelems_ = 0;
  }
}

void PingPongBuffer::bind() {
  // Bind attributes for the ping pong buffers.
  // Assume attributes are sequentially laid out in the same buffer (AOS)  ??
  // and with identic attribute size.

  if (bUseSOALayout_) {
    // SoA
    for (int i = 0; i < kNumBuffers; ++i) {
      int const first_binding = i * nattribs_ + base_binding_;
      int const buffer = device_storage_ids_[i];
      for (int j=0; j < nattribs_; ++j) {
        int const binding = j + first_binding;
        int const offset  = j * attrib_buffer_bytesize_;
        glBindBufferRange( GL_SHADER_STORAGE_BUFFER, binding, buffer, offset, attrib_buffer_bytesize_);
      }
    }
  } 
  else { 
    // AoS
    glBindBuffersBase( GL_SHADER_STORAGE_BUFFER, base_binding_, kNumBuffers, device_storage_ids_.data()); //
  }

  CHECK_GX_ERROR();
}

void PingPongBuffer::unbind() {
  if (bUseSOALayout_) { 
    // SoA
    for (int i = 0; i < kNumBuffers; ++i) {
      int const first_binding = base_binding_ + i * nattribs_;
      glBindBuffersBase( GL_SHADER_STORAGE_BUFFER, first_binding, nattribs_, nullptr);
    }
  } else {
    // AoS
    glBindBuffersBase( GL_SHADER_STORAGE_BUFFER, base_binding_, kNumBuffers, nullptr); //
  }
}

void PingPongBuffer::swap() {
  assert(kNumBuffers == 2);

  // Costly but safe. 
  glCopyNamedBufferSubData(
    device_storage_ids_[1u], 
    device_storage_ids_[0u],
    0, 
    0, 
    storage_buffer_bytesize_
  );
}

GLuint PingPongBuffer::storage_buffer_id(int id) const {
  return device_storage_ids_[id % kNumBuffers];   //
}

// ----------------------------------------------------------------------------
