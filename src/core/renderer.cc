#include "core/renderer.h"
#include "core/global_clock.h"

#include "ui/views/views.h"

// ----------------------------------------------------------------------------

Renderer::~Renderer() {
  particle_.deinit();
  hair_.deinit();
  grid_.deinit();
  skybox_.deinit();
  gizmo_.deinit();
  postprocess_.deinit();
}

void Renderer::init() {
  postprocess_.init();
  gizmo_.init();
  grid_.init();
  skybox_.init();
  particle_.init();  
  hair_.init();

  ui_view = std::make_shared<views::RendererView>(params_);
}

void Renderer::frame(SceneHierarchy &scene, Camera &camera, UpdateCallback_t update_cb, DrawCallback_t draw_cb) {
  float const dt = GlobalClock::Get().deltaTime();

  gizmo_.begin_frame( dt, camera);

  // User's update.
  update_cb(); //

  // [ should this be updated before or after user's ? ]
  camera.update(dt);
  scene.update(dt, camera);

  // UPDATE
  {
    // Postprocessing, resize textures when needed [to improve]
    postprocess_.setup_textures(camera); //

    // Grid.
    grid_.update(dt, camera);

    // Particles.
    if (params_.enable_particle) {
      particle_.update( dt, camera);
    }
    
    // Hair.
    if (params_.enable_hair && hair_.initialized()) {
      if (auto const& colliders = scene.colliders(); !colliders.empty()) {
        auto const& e = colliders.front();
        auto const& bsphere = e->get<SphereColliderComponent>();

        // [debug bounding sphere]
        auto bsParams = scene.globalMatrix(e->index()) * glm::vec4(bsphere.center(), 1.0);
        bsParams.w = bsphere.radius();
        hair_.set_bounding_sphere( bsParams );  
      }
      hair_.update(dt);
    }
  }
  CHECK_GX_ERROR();

  // RENDER
  {
    gx::Viewport( camera.width(), camera.height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //

    // 'Deferred'-pass, post-process the solid objects.
   
    postprocess_.begin();
    drawPass( RendererPassBit::PASS_DEFERRED, scene, camera);
    postprocess_.end(camera);

    // Forward-pass, render the special effects.
    drawPass( RendererPassBit::PASS_FORWARD, scene, camera); // [to tonemap !]

    // [ should have a final composition pass here to tonemap the forwards ].
  }
  CHECK_GX_ERROR();
  
  // User's draw.
  draw_cb(); //

  scene.gizmos(false); //

  gizmo_.end_frame(camera);
}

// void Renderer::register_draw_cb(DrawCallback_t const& draw_cb) {
//   draw_callbacks_.push_back(draw_cb);
// }

// ----------------------------------------------------------------------------

void Renderer::drawPass(RendererPassBit bitmask, SceneHierarchy const& scene, Camera const& camera) {
  // Reset default states.
  gx::PolygonMode( gx::Face::FrontAndBack, gx::RenderMode::Fill);
  gx::Disable( gx::State::Blend );
  gx::Enable( gx::State::DepthTest );
  gx::DepthMask( true );
  gx::Enable( gx::State::CullFace );
  gx::CullFace( gx::Face::Back );
  gx::Enable( gx::State::CubeMapSeamless );

  // -------------------------
  // -- SKYBOX
  // -------------------------
  if (bitmask & SCENE_SKYBOX_BIT)
  {
    // We could disable depth testing here when the skybox is always the
    // first rendered object, which is supposed to, but it depends on the pass.
    gx::Disable( gx::State::DepthTest );
    gx::CullFace( gx::Face::Front );
    //gx::Enable( gx::State::CubeMapSeamless );
    gx::DepthMask( false );

    if (params_.show_skybox) {
      skybox_.render(camera);
    }

    gx::DepthMask( true );
    //gx::Disable( gx::State::CubeMapSeamless );
    gx::CullFace( gx::Face::Back );
    gx::Enable( gx::State::DepthTest );
  }
  CHECK_GX_ERROR();

  // -------------------------
  // -- OPAQUE OBJECTS (front to back)
  // -------------------------
  if (bitmask & SCENE_OPAQUE_BIT)
  {
    if (!params_.show_wireframe) {
      drawEntities( RenderMode::Opaque, scene, camera );
      drawEntities( RenderMode::CutOff, scene, camera );
    }
  }
  CHECK_GX_ERROR();

  // -------------------------
  // -- WIRE OBJECTS
  // -------------------------
  if (bitmask & SCENE_WIRE_BIT)
  {
    gx::PolygonMode( gx::Face::FrontAndBack, gx::RenderMode::Line );

    if (params_.show_wireframe) {
      drawEntities( RenderMode::Opaque, scene, camera );
      drawEntities( RenderMode::CutOff, scene, camera );

      if (params_.enable_hair) {
        hair_.render(camera);
      }
    }

    gx::PolygonMode( gx::Face::FrontAndBack, gx::RenderMode::Fill );
  }
  CHECK_GX_ERROR();

  // -------------------------
  // -- HAIR
  // -------------------------
  if (bitmask & SCENE_HAIR_BIT)
  {
    gx::Disable( gx::State::CullFace );
    if (!params_.show_wireframe && params_.enable_hair) {
      hair_.render(camera);
    }
    gx::Enable( gx::State::CullFace );
  }
  CHECK_GX_ERROR();
  
  // -------------------------
  // -- PARTICLES
  // -------------------------
  if (bitmask & SCENE_PARTICLE_BIT)
  {
    gx::DepthMask( false );
    gx::Enable( gx::State::Blend );
    gx::Disable( gx::State::CullFace );

    if constexpr (false) {
      gx::BlendFunc( gx::BlendFactor::SrcAlpha, gx::BlendFactor::One);
      particle_.set_sorting(false);
    } else {
      gx::BlendFunc( gx::BlendFactor::SrcAlpha, gx::BlendFactor::OneMinusSrcAlpha);
      particle_.set_sorting(true);
    }

    // Particles.
    if (params_.enable_particle) {
      particle_.render(camera);
    }

    gx::Disable( gx::State::Blend );
    gx::DepthMask( true );
  }
  CHECK_GX_ERROR();

  // -------------------------  
  // -- TRANSPARENT OBJECTS (back to front)
  // -------------------------
  if (bitmask & SCENE_TRANSPARENT_BIT)
  {
    gx::Enable( gx::State::Blend );
    gx::BlendFunc( gx::BlendFactor::SrcAlpha, gx::BlendFactor::OneMinusSrcAlpha);
    gx::DepthMask( false );

    // Grid.
    if (params_.show_grid) {
      grid_.render(camera); //
    }

    // Double faced meshes (work better when the mesh is convex).
    gx::Enable( gx::State::CullFace );
    {
      gx::CullFace( gx::Face::Front );
      drawEntities( RenderMode::Transparent, scene, camera);
  
      gx::CullFace( gx::Face::Back );
      drawEntities( RenderMode::Transparent, scene, camera);
    }
    gx::Disable( gx::State::CullFace );

    gx::DepthMask( true );
    gx::Disable( gx::State::Blend );
  }
  CHECK_GX_ERROR();

  // -------------------------
  // -- DEBUG OBJECTS
  // -------------------------
  if (bitmask & SCENE_DEBUG_BIT)
  {
    if (params_.enable_particle) {
      //particle_.render_debug_particles(camera);
    }

    // Display rigs with debug shapes.
    if (params_.show_rigs) {
      scene.renderDebugRigs();
    }

    // Display colliders with debug shapes.
    scene.renderDebugColliders();
  }

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------

void Renderer::drawEntities(RenderMode render_mode, SceneHierarchy const& scene, Camera const& camera) {
  auto render_drawables = [this, render_mode, &scene, &camera](EntityHandle drawable) {
    // global matrix of the entity.
    auto const& world = scene.globalMatrix(drawable->index());

    // External, per-mesh render attributes.
    RenderAttributes attributes;
    // (vertex)
    attributes.mvp_matrix          = camera.viewproj() * world;
    attributes.world_matrix        = world;

    // (vertex skinning)
    if (drawable->has<SkinComponent>()) {
      auto const& skin = drawable->get<SkinComponent>();
      attributes.skinning_texid    = skin.texture_id(); //
      attributes.skinning_mode     = skin.skinning_mode();
    }

    // (fragment)
    attributes.brdf_lut_texid      = skybox_.textureBRDFLookup()->id;
    attributes.prefilter_texid     = skybox_.texturePrefilter() ? skybox_.texturePrefilter()->id : 0u;
    attributes.irradiance_texid    = skybox_.textureIrradiance() ? skybox_.textureIrradiance()->id : 0u;
    attributes.irradiance_matrices = skybox_.hasIrradianceMatrices() ? skybox_.irradianceMatrices() : nullptr;
    attributes.eye_position        = camera.position();
    //attributes.tonemap_mode      = tonemap_mode; // [todo]

    // Rendering.
    auto &visual = drawable->get<VisualComponent>();
    visual.render( attributes, render_mode );
  };

  auto drawables = scene.drawables();

  // Choose traversal depending on the RenderMode.
  if (RenderMode::Transparent == render_mode) {
    // Back to Front.
    std::for_each(drawables.rbegin(), drawables.rend(), render_drawables);
  } else {
    // Front to Back.
    std::for_each(drawables.begin(), drawables.end(), render_drawables);
  }

  gx::UseProgram();
  gx::UnbindTexture();

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------
