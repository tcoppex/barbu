#ifndef BARBU_FX_HAIR_H_
#define BARBU_FX_HAIR_H_

#include <vector>

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

#include "core/graphics.h"
#include "fx/marschner.h"
#include "memory/assets/mesh.h"     // (to minimize)
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
  // Parameters.
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

    UIView *ui_marschner = nullptr;
  };

 public:
  Hair()
    : nroots_(0)
  {}

  void init(ResourceId const& scalpId);
  void deinit();

  void update(float const dt);
  void render(Camera const& camera);

  void set_bounding_sphere(glm::vec4 const& bsphere) {
    boundingsphere_ = bsphere;
  }

  UIView* view() const;

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

  glm::vec4 boundingsphere_;

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
    GLuint cs_simulation;
    GLuint tess_stream;
    GLuint render;
    GLuint render_debug;
  } pgm_;                           //< Hair pipeline program shaders.

  struct {
    UIView *hair = nullptr;
  } ui_views_;                      //< UI views.
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_HAIR_H_