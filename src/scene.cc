#include "scene.h"

#include "core/events.h"
#include "core/logger.h"
#include "memory/assets/assets.h"
#include "ui/views/views.h"

// ----------------------------------------------------------------------------

void Scene::init(Camera const& camera, views::Main &ui_mainview) {
  // OpenGL rendering parameters.
  gx::ClearColor(0.0165f, 0.0165f, 0.0160f, 1.0f);
  //gx::ClearColor(0.9f, 0.7f, 0.5f, 1.0f);

  scene_hierarchy_.init();
  //scene_hierarchy_.add_bounding_sphere();

  // Special Rendering.
  skybox_.init();
  grid_.init();

  // Simulation FX.
  particle_.init();

  // Sample scene.
  {
    #if 0
    // Model.
    scene_hierarchy_.import_model( 
      ASSETS_DIR "/models/InfiniteScan/Head.glb" 
    );

    // Scalp.
    // [todo: extract from the GLTF model directly]
    auto const scalpId = AssetId( 
      ASSETS_DIR "/models/InfiniteScan/Head_scalp.obj" 
    );
    hair_.init( scalpId ); //
    #else
    scene_hierarchy_.import_model( 
      ASSETS_DIR "/models/gltf_samples/RiggedFigure.glb" 
    );
    params_.enable_hair = false;
    #endif
  }
  
  // UI Views.
  setup_ui_views(ui_mainview);

  CHECK_GX_ERROR();
}

void Scene::deinit() {
  delete ui_view_;

  particle_.deinit();
  hair_.deinit();
  grid_.deinit();
  skybox_.deinit();
}

UIView* Scene::view() const {
  return ui_view_;
}

void Scene::update(float const dt, Camera &camera) {
  auto const& eventData{ GetEventData() };

  // Display opaque materials wireframe.
  if ('w' == eventData.lastChar) {
    params_.show_wireframe ^= true;
  }

  auto &selected = scene_hierarchy_.selected();
  if (!selected.empty()) {
    // Reset.
    if ('x' == eventData.lastChar) {
      for (auto &e : selected) {
        scene_hierarchy_.reset_entity(e);
      }
    } 
    // Delete.
    else if ('X' == eventData.lastChar) {
      for (auto &e : selected) {
        scene_hierarchy_.remove_entity(e);
      }
    }
  }
  
  switch (eventData.lastChar) {
    // Select  / Unselect all.
    case 'a':
      scene_hierarchy_.select_all(selected.empty());
    break;
    
    // Focus on entities.
    case 'C':
      ((ArcBallController*)camera.controller())->set_target(scene_hierarchy_.pivot());
    break;
    case 'c':
      ((ArcBallController*)camera.controller())->set_target(scene_hierarchy_.centroid());
    break;
    case GLFW_KEY_1:
    case GLFW_KEY_3:
    case GLFW_KEY_7:
      ((ArcBallController*)camera.controller())->set_target(scene_hierarchy_.centroid(), true);
    break;
  }

  // Import drag-n-dropped objects & center them to camera target.
  for (auto &fn : eventData.dragFilenames) {
    auto const ext = fn.substr(fn.find_last_of(".") + 1);
    if (MeshDataManager::CheckExtension(ext)) {
      if (auto e = scene_hierarchy_.import_model(fn); e) {
        e->set_position( camera.target() );
      }
    }
  }

  // Update sub-systems.
  scene_hierarchy_.update(dt, camera);
  grid_.update(dt, camera);

  if (params_.enable_particle) {
    particle_.update(dt, camera);
  }
  if (params_.enable_hair) {
    hair_.update(dt);
  }
}

void Scene::render(Camera const& camera, uint32_t bitmask) {
  // Reset default states.
  gx::PolygonMode( gx::Face::FrontAndBack, gx::RenderMode::Fill);
  gx::Disable( gx::State::Blend );
  gx::Enable( gx::State::DepthTest );
  gx::DepthMask( true );
  gx::Enable( gx::State::CullFace );
  gx::CullFace( gx::Face::Back );

  // -------------------------
  // -- SKYBOX
  // -------------------------
  if (bitmask & SCENE_SKYBOX_BIT)
  {
    // We could disable depth testing here when the skybox is always the
    // first rendered object, which is supposed to, but it depends on the pass.
    gx::Disable( gx::State::DepthTest );
    gx::CullFace( gx::Face::Front );
    gx::Enable( gx::State::CubeMapSeamless );
    gx::DepthMask( false );

    if (params_.show_skybox) {
      skybox_.render(camera);
    }

    gx::DepthMask( true );
    gx::Disable( gx::State::CubeMapSeamless );
    gx::CullFace( gx::Face::Back );
    gx::Enable( gx::State::DepthTest );
  }
  CHECK_GX_ERROR();

  // -------------------------
  // -- OPAQUE OBJECTS (front to back)
  // -------------------------
  if (bitmask & SCENE_SOLID_BIT)
  {
    if (!params_.show_wireframe) {
      render_entities( RenderMode::Opaque, camera );
      render_entities( RenderMode::CutOff, camera );
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
      render_entities( RenderMode::Opaque, camera );
      render_entities( RenderMode::CutOff, camera );

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

    if constexpr (true) {
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
      grid_.render(camera);
    }

    // Double faced meshes.
    gx::Enable( gx::State::CullFace );
    {
      gx::CullFace( gx::Face::Front );
      render_entities( RenderMode::Transparent, camera);
  
      gx::CullFace( gx::Face::Back );
      render_entities( RenderMode::Transparent, camera);
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
      //render_debug_particles(camera);
    }

    // Show Gizmo.
    if (params_.show_transform) {
      // 1) Use global matrices for gizmos.
      for (auto e : scene_hierarchy_.selected()) {
        auto &global = scene_hierarchy_.global_matrix( e->index() );
        Im3d::Gizmo( e->name().c_str(), glm::value_ptr( global ));
      }
      // 2) Recompute selected locals from modified globals.
      scene_hierarchy_.update_selected_local_matrices();
    }
  }

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------

void Scene::setup_ui_views(views::Main &ui_mainview) {
  // [to rework, the UI system is confusing]

  ui_view_ = new views::SceneView(params_);
  ui_mainview.push_view( ui_view_ );

  if (nullptr != scene_hierarchy_.ui_view) {
    //ui_mainview.push_view( scene_hierarchy_.ui_view );
    params_.ui_view_ptr = scene_hierarchy_.ui_view;
  }

  if (nullptr != hair_.view()) {
    ui_mainview.push_view( hair_.view() );
  }

  if (nullptr != particle_.view()) {
    //ui_mainview.push_view( particle_.view() );
  }
}

// ----------------------------------------------------------------------------

void Scene::render_entities(RenderMode render_mode, Camera const& camera) {
  auto render_drawables = [this, render_mode, &camera](EntityHandle &drawable) {
    // global matrix of the entity.
    auto const& world = scene_hierarchy_.global_matrix( drawable->index() );

    // External, per-mesh render attributes.
    RenderAttributes attributes;
    attributes.irradiance_matrices = skybox_.irradiance_matrices();
    attributes.world_matrix        = world;
    attributes.mvp_matrix          = camera.viewproj() * world;
    attributes.eye_position        = camera.position(); // !!

    // visual parameters.
    auto &visual = drawable->get<VisualComponent>();
    visual.render( attributes, render_mode );
  };

  auto drawables = scene_hierarchy_.drawables();

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


#if 0

void Scene::render_debug_particles(Camera const& camera) {
  auto const& params = particle_.simulation_parameters();
  float const radius = params.emitter_radius;

  glm::vec3 scale(1.0f);

  switch (params.emitter_type) {
    case GPUParticle::EMITTER_DISK:
      scale = glm::vec3(radius, 0.0f, radius);
    break;

    case GPUParticle::EMITTER_SPHERE:
    case GPUParticle::EMITTER_BALL:
      scale = glm::vec3(radius);
    break;

    case GPUParticle::EMITTER_POINT:
    default:
    break;
  }

  gx::Disable( gx::State::CullFace );
  gx::PolygonMode( gx::Face::FrontAndBack, gx::RenderMode::Line);

  gx::UseProgram( pgm_handle_->id );
  {
    // emitter.
    glm::mat4 model = glm::translate(glm::mat4(1.0f), params.emitter_position)
                    * glm::scale(glm::mat4(1.0f), scale);
    glm::vec4 color = glm::vec4(0.9f, 0.9f, 1.0f, 0.5f);
    draw_mesh( debug_sphere_, model, color, camera);

    // bounding volume.
    model = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f*params.bounding_volume_size));
    color = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);
    draw_mesh( debug_sphere_, model, color, camera);
  }
  gx::UseProgram();

  gx::PolygonMode( gx::Face::FrontAndBack, gx::RenderMode::Fill);
  gx::Enable( gx::State::CullFace );
}

#endif

// ----------------------------------------------------------------------------
