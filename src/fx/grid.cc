#include "fx/grid.h"

#include "core/camera.h"
#include "core/graphics.h"
#include "utils/arcball_controller.h"
#include "memory/assets/assets.h"

// ----------------------------------------------------------------------------

void Grid::init() {
  mesh_ = MESH_ASSETS.createGrid(kGridNumCell);

  // Shader Program.  
  pgm_ = PROGRAM_ASSETS.createRender( 
    "Program::Grid",
    SHADERS_DIR "/grid/vs_grid.glsl",
    SHADERS_DIR "/grid/fs_grid.glsl"
  );

  CHECK_GX_ERROR();
}

void Grid::deinit() {
  mesh_.reset();
}

void Grid::update(float const dt, Camera const& camera) {
  // Reset rendering params.
  matrix_ = glm::mat4(1.0f);

  if (!kEnableSideGrid) {
    return;
  }

  float constexpr epsX = 0.075f;
  float constexpr epsY = 0.150f;

  auto const up    = glm::vec3(0.0, 1.0, 0.0);
  auto const front = camera.direction();

  // SIDE-VIEW grid rendering.
  auto const* abc = (ArcBallController const*)camera.controller(); //

  // Blend the grid only when centered on the y-axis.
  auto const dp = glm::abs(glm::dot(front, up));
  float factor = (glm::abs(abc->target().y) < 1.0e-5f) ? glm::smoothstep(0.0f, 0.75f*epsX, dp) : 1.0f;
  
  if (abc->is_sideview_set())
  {
    auto const afront = glm::abs(front);

    float constexpr half_epsX = 0.5f * epsX;
    float constexpr anti_epsY = 1.0f - epsY;
    
    bool const bNotOnSide = !((afront.x > epsY) && (afront.x < anti_epsY))
                         && !((afront.y > epsY) && (afront.y < anti_epsY))
                         && !((afront.z > epsY) && (afront.z < anti_epsY))
                         ;

    if (dp >= half_epsX) {
      factor = (bNotOnSide) ? 1.0f - (glm::smoothstep(0.f, half_epsX, dp) - glm::smoothstep(half_epsX, epsX, dp))
                            : factor;
    } else if (bNotOnSide) {
      auto right = glm::cross(up, front);

      // saturate the right axis near an epsilon. 
      right   = glm::sign(right) * glm::step(glm::vec3(anti_epsY), glm::abs(right));
      matrix_ = glm::rotate(matrix_, glm::pi<float>() / 2.0f, right);

      factor = glm::mix(
        glm::smoothstep(anti_epsY, 1.0f, 1.0f-glm::abs(glm::dot(right, front))),
        glm::smoothstep(1.0f-half_epsX, 1.0f, 1.0f-dp),
        0.5f
      );
    }
  }

  alpha_ = glm::mix(0.0f, kGridAlpha, factor);
}

void Grid::render(Camera const& camera) {
  float const kGridSize = kGridScale * static_cast<float>(kGridNumCell);

  glm::vec4 const color(glm::vec3(kGridValue), alpha_); // 

  auto const pgm = pgm_->id;
  gx::SetUniform( pgm, "uModel",       matrix_);
  gx::SetUniform( pgm, "uViewproj",    camera.viewproj());
  gx::SetUniform( pgm, "uColor",       color);
  gx::SetUniform( pgm, "uScaleFactor", kGridSize);
  
  glEnable( GL_LINE_SMOOTH ); //
  glUseProgram(pgm);
    mesh_->draw();
  glUseProgram(0u);
  glDisable( GL_LINE_SMOOTH );

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------
