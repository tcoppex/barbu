#include "utils/gizmo.h"

#include "im3d/im3d_math.h"
#include "glm/gtc/type_ptr.hpp"

#include "core/camera.h"
#include "core/events.h"
#include "memory/assets/assets.h"

using namespace Im3d;

// This code is based off Im3D example from John Chapman.

// ----------------------------------------------------------------------------

void Gizmo::init() {
  pgm_.points = PROGRAM_ASSETS.createRender(
    "gizmo::point",
    SHADERS_DIR "/im3d/vs_points.glsl",
    SHADERS_DIR "/im3d/fs_points.glsl"
  )->id;

  pgm_.lines = PROGRAM_ASSETS.createGeo(
    "gizmo::line",
    SHADERS_DIR "/im3d/vs_lines.glsl",
    SHADERS_DIR "/im3d/gs_lines.glsl",
    SHADERS_DIR "/im3d/fs_lines.glsl"
  )->id;

  pgm_.triangles = PROGRAM_ASSETS.createRender(
    "gizmo::triangle",
    SHADERS_DIR "/im3d/vs_triangles.glsl",
    SHADERS_DIR "/im3d/fs_triangles.glsl"
  )->id;


  // -- Attributes.
  glCreateBuffers( 1u, &vbo_);
  glCreateVertexArrays(1, &vao_);

  int binding(-1), attrib(-1);
  glBindVertexArray(vao_);
  {
    glBindVertexBuffer(++binding, vbo_, 0, sizeof(Im3d::VertexData));
    // Position.
    glVertexAttribFormat(++attrib, 4, GL_FLOAT, GL_FALSE, (GLuint)offsetof(Im3d::VertexData, m_positionSize));
    glVertexAttribBinding(attrib, binding);
    glEnableVertexAttribArray(attrib);
    // Color.
    glVertexAttribFormat(++attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, (GLuint)offsetof(Im3d::VertexData, m_color));
    glVertexAttribBinding(attrib, binding);
    glEnableVertexAttribArray(attrib);
  }
  glBindVertexArray(0);

  CHECK_GX_ERROR();
}

void Gizmo::deinit() {
  glDeleteVertexArrays(1, &vao_);
  glDeleteBuffers(1, &vbo_);
}

void Gizmo::begin_frame(float dt, Camera const& camera) {
  IM3D_ASSERT(vao_ && vbo_ && pgm_.points && pgm_.lines && pgm_.triangles);

// At the top of each frame, the application must fill the Im3d::AppData struct and then call Im3d::NewFrame().
// The example below shows how to do this, in particular how to generate the 'cursor ray' from a mouse position
// which is necessary for interacting with gizmos.

  TEventData const event = GetEventData();

  AppData& ad = GetAppData();

  ad.m_deltaTime     = dt;
  ad.m_viewportSize  = Vec2((float)camera.width(), (float)camera.height());
  ad.m_viewOrigin    = Vec3(camera.position());
  ad.m_viewDirection = Vec3(camera.direction());
  ad.m_worldUp       = Vec3(0.0f, 1.0f, 0.0f);
  ad.m_projOrtho     = camera.is_ortho(); 
  ad.m_flipGizmoWhenBehind = kbFlipGizmoWhenBehind;

  auto const W = camera.proj()[0][0];
  auto const H = camera.proj()[1][1];

 // m_projScaleY controls how gizmos are scaled in world space to maintain a constant screen height
  ad.m_projScaleY = camera.is_ortho()
    ? 2.0f / H
    : tanf(camera.fov() * 0.5f) * 2.0f
    ;  
  ad.m_projScaleY *= kGizmoScale;

  Vec2 cursorPos(event.mouseX, event.mouseY);
  cursorPos = (cursorPos / ad.m_viewportSize) * 2.0f - Vec2(1.0f);
  cursorPos.y = -cursorPos.y;
  
  auto const worldMatrix = Mat4(camera.world());

  Vec3 rayOrigin, rayDirection;
  if (camera.is_ortho())
  {
    rayOrigin.x  = cursorPos.x / W;
    rayOrigin.y  = cursorPos.y / H;
    rayOrigin.z  = 0.0f;
    rayOrigin    = worldMatrix * Vec4(rayOrigin, 1.0f);
    rayDirection = worldMatrix * Vec4(0.0f, 0.0f, -1.0f, 0.0f); 
  }
  else
  {
    rayOrigin = ad.m_viewOrigin;
    rayDirection.x  = cursorPos.x / W;
    rayDirection.y  = cursorPos.y / H;
    rayDirection.z  = -1.0f;
    rayDirection    = worldMatrix * Vec4(Normalize(rayDirection), 0.0f);
  }
  ad.m_cursorRayOrigin = rayOrigin;
  ad.m_cursorRayDirection = rayDirection;

 // Set cull frustum planes. This is only required if IM3D_CULL_GIZMOS or IM3D_CULL_PRIMTIVES is enable via
 // im3d_config.h, or if any of the IsVisible() functions are called.
  //ad.setCullFrustum(g_Example->m_camViewProj, true);

  bool const bSelect           = event.bLeftMouse;
  bool const bSnap             = event.bLeftCtrl; 
  bool const bGizmoTranslation = (event.lastChar == 't');
  bool const bGizmoRotation    = (event.lastChar == 'r');
  bool const bGizmoScale       = (event.lastChar == 's');

  // Handle switching from local / global space.
  // ---------------
  bool const bGizmoLocal = ((bLastTranslation_ && bGizmoTranslation) 
                         || (bLastRotation_ && bGizmoRotation)) == kDefaultGlobal;

  auto constexpr switch_local = [](bool &a, bool b0, bool b1, bool b2) {
    a = b0 ? true : a && !b1 && !b2;
  };
  switch_local(bLastTranslation_, bGizmoTranslation, bGizmoRotation, bGizmoScale);
  switch_local(bLastRotation_,    bGizmoRotation, bGizmoTranslation, bGizmoScale);
  // ---------------

  ad.m_keyDown[Im3d::Action_Select]           = bSelect;
  ad.m_keyDown[Im3d::Action_GizmoLocal]       = bGizmoLocal;
  ad.m_keyDown[Im3d::Action_GizmoTranslation] = bGizmoTranslation;
  ad.m_keyDown[Im3d::Action_GizmoRotation]    = bGizmoRotation;
  ad.m_keyDown[Im3d::Action_GizmoScale]       = bGizmoScale;

  // [ Snapping with local transform bug ]
  ad.m_snapTranslation = bSnap ? kTranslationSnapUnit : 0.0f;
  ad.m_snapRotation    = bSnap ? kRotationSnapUnit : 0.0f;
  ad.m_snapScale       = bSnap ? kScalingSnapUnit : 0.0f;

  Im3d::SetAlpha(0.66);
  Im3d::NewFrame();
}

void Gizmo::end_frame(Camera const& camera) {
// After all Im3d calls have been made for a frame, the user must call Im3d::EndFrame() to finalize draw data, then
// access the draw lists for rendering. Draw lists are only valid between calls to EndFrame() and NewFrame().
// The example below shows the simplest approach to rendering draw lists; variations on this are possible. See the 
// shader source file for more details.

  Im3d::EndFrame();

  AppData& ad = GetAppData();

 // Primitive rendering.
  glViewport(0, 0, (GLsizei)ad.m_viewportSize.x, (GLsizei)ad.m_viewportSize.y);//
  gx::Enable(gx::State::Blend);
  glBlendEquation(GL_FUNC_ADD);
  gx::BlendFunc( gx::BlendFactor::SrcAlpha, gx::BlendFactor::OneMinusSrcAlpha);
  
  //glEnable( GL_PROGRAM_POINT_SIZE ); //

  gx::Disable( gx::State::DepthTest );
  gx::DepthMask( false );

  for (uint32_t i = 0, n = Im3d::GetDrawListCount(); i < n; ++i)
  {
    const Im3d::DrawList& drawList = Im3d::GetDrawLists()[i];
 
    // if (drawList.m_layerId == Im3d::MakeId("NamedLayer"))
    // {
    //   // The application may group primitives into layers, which can be used to 
    //   // change the draw state (e.g. enable depth testing, use a different shader)
    // }
  
    GLenum prim;
    GLuint sh;
    switch (drawList.m_primType)
    {
      case Im3d::DrawPrimitive_Points:
        prim = GL_POINTS;
        sh = pgm_.points;
        gx::Disable(gx::State::CullFace); // points are view-aligned
      break;
      
      case Im3d::DrawPrimitive_Lines:
        prim = GL_LINES;
        sh = pgm_.lines;
        gx::Disable(gx::State::CullFace); // lines are view-aligned
        gx::SetUniform( sh, "uViewport", glm::vec2(ad.m_viewportSize));
      break;

      case Im3d::DrawPrimitive_Triangles:
        prim = GL_TRIANGLES;
        sh = pgm_.triangles;
        //gx::Enable(gx::State::CullFace); // culling valid for triangles, but optional
      break;
      
      default:
        IM3D_ASSERT(false);
      return;
    };
  
    glBindVertexArray(vao_);

    const auto bytesize = (GLsizeiptr)drawList.m_vertexCount * sizeof(Im3d::VertexData);
    glNamedBufferData(vbo_, bytesize, nullptr, GL_STREAM_DRAW);
    glNamedBufferSubData(vbo_, 0, bytesize, (GLvoid*)drawList.m_vertexData);

    gx::SetUniform( sh, "uViewProjMatrix", camera.viewproj());

    gx::UseProgram(sh);    
    glDrawArrays(prim, 0, (GLsizei)drawList.m_vertexCount);
  }
  gx::UseProgram();
  glBindVertexArray(0);
  
  gx::Enable(gx::State::DepthTest);
  gx::DepthMask( true );

  // Text rendering.
  //g_Example->drawTextDrawListsImGui(Im3d::GetTextDrawLists(), Im3d::GetTextDrawListCount());

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------
