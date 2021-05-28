#ifndef BARBU_FX_HAIR_H_
#define BARBU_FX_HAIR_H_

#include <vector>

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

#include "core/graphics.h"
#include "fx/marschner.h"
#include "memory/assets/mesh.h"
#include "memory/assets/program.h"
#include "memory/pingpong_buffer.h"
#include "memory/random_buffer.h"

class Camera;
class UIView;

// ----------------------------------------------------------------------------

//
//  Render Hair on the GPU.
//
class Hair {
 public:
  // UI Parameters.
  struct Parameters_t {
    struct {
      float maxlength  = 0.50f;
    } sim;

    struct {
      int ninstances   = 3;
      int nlines       = 2;
      int nsubsegments = 16;
    } tess;

    struct {
      glm::vec3 albedo  = glm::vec3(0.33, 0.32, 0.30);
      float linewidth   = 0.014f;
      float lengthScale = 1.450f;
      bool bShowDebugCP = false;
    } render;

    struct {
      int nroots;
      int nControlPoints;
    } readonly;

    std::shared_ptr<UIView> ui_marschner = nullptr;
  };

  std::shared_ptr<UIView> ui_view = nullptr;

 public:
  Hair()
    : nroots_(0)
  {}

  /* Initialize base resources & UI. */
  void init();

  /* Release all allocated resources. */
  void deinit();

  /* Setup scalp specific resources. */
  void setup(ResourceId const& scalpId);

  void update(float const dt);
  void render(Camera const& camera);

  inline void set_bounding_sphere(glm::vec4 const& bsphere) noexcept {
    boundingsphere_ = bsphere;
  }

  inline bool initialized() const noexcept {
    return nroots_ != 0;
  }

 private:
  void init_simulation(MeshData const& scalpMesh);
  void init_mesh(MeshData const& scalpMesh);
  void init_transform_feedbacks();
  void init_shaders();
  void init_ui_views();

  Parameters_t params_;             //< Parameters for simulation & rendering.

  int nroots_;                      //< Number of strands / roots vertices in the scalp.
  PingPongBuffer pbuffer_;          //< Strands 'particle' control points datas.
  std::vector<glm::vec3> normals_;  //< (TMP) base normals at each roots.

  RandomBuffer randbuffer_;         //< Buffer of random values used by the tesselator.

  Marschner marschner_;             //< Handle data generation for the Marschner 
                                    //  shading reflectance model.
  
  glm::mat4 model_; //
  glm::vec4 boundingsphere_; //

  struct {
    GLuint vao;
    GLuint ibo;
    int nelems;
    int patchsize;
  } mesh_;                          //< VAO to render the pbuffer.

  struct {
    GLuint tf;
    GLuint strands_buffer_id;
    GLuint vao;
  } tess_stream_;                   //< Transform feedback parameters for tess output.

  struct {
    ProgramHandle cs_simulation;
    ProgramHandle tess_stream;
    ProgramHandle render;
    ProgramHandle render_debug;
  } pgm_;                           //< Hair pipeline program shaders.

};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_HAIR_H_