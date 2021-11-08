#ifndef BARBU_PING_PONG_BUFFER_H_
#define BARBU_PING_PONG_BUFFER_H_

#include "core/graphics.h"
#include <array>

// ----------------------------------------------------------------------------

//
// Implementation of a 'ping-pong' doouble-buffer on the GPU.
//
// A buffer contains subsequent attributes, either as Structure Of Arrays or Array
// of structure layout, all of size float4 (128 bytes).
//
// (this might change for single attributes of custom size per pingpong buffer in future version)
//
class PingPongBuffer {
 public:
  static constexpr int kNumBuffers = 2;
  static constexpr int kAttribBytesize = 4 * sizeof(float);

  template<typename T>
  static constexpr int NumAttribsRequired() {
    return (sizeof(T) + kAttribBytesize - 1u) / kAttribBytesize;
  } 

  PingPongBuffer()
    : nelems_(0)
    , nattribs_(0)
    , attrib_buffer_bytesize_(0)
    , storage_buffer_bytesize_(0)
    , base_binding_(0)
    , base_buffer_id_(0)
  {}

  ~PingPongBuffer() {
    destroy();
  }

  void setup(int const nelems, int const base_binding, int const nattribs=1, bool const bUseSOALayout=true);
  void destroy();

  void bind();
  void unbind();

  void swap();

  GLuint storage_buffer_id(int const id) const;
  GLuint read_ssbo_id() const { return storage_buffer_id(0); }
  GLuint write_ssbo_id() const { return storage_buffer_id(1); }

  int                    size() const noexcept { return nelems_; }
  int           num_atributes() const noexcept { return nattribs_; }
  int  attrib_buffer_bytesize() const noexcept { return attrib_buffer_bytesize_; }
  int storage_buffer_bytesize() const noexcept { return storage_buffer_bytesize_; }

  uint32_t attrib_index(uint32_t attrib_bind) const noexcept {
    return attrib_bind - base_binding_;
  }

 private:
  int nelems_;
  int nattribs_;
  int attrib_buffer_bytesize_;
  int storage_buffer_bytesize_;

  bool bUseSOALayout_; // [use an enum instead]
  
  int base_binding_;
  int base_buffer_id_;
  std::array<GLuint, kNumBuffers> device_storage_ids_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_PING_PONG_BUFFER_H_