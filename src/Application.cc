#include "Application.h"

#include "memory/assets/assets.h"
#include "ui/views/views.h"

// ----------------------------------------------------------------------------

void Application::setup() {
  // Gamma-corrected clear color.
  gx::ClearColor(0.15f, 0.12f, 0.12f, true);

  // Camera.
  {
    float const rx{ + glm::pi<float>() / 16.0f };
    float const ry{ + glm::pi<float>() / 8.0f };
    arcball_controller_.set_view( rx, ry, false);
    arcball_controller_.set_dolly(16.0f, false);

    camera_.set_controller(&arcball_controller_);
    camera_.set_perspective(glm::radians(60.0f), resolution(), 0.01f, 500.0f);
  }

  // Skybox texture.
  renderer_.skybox().setup_texture(
    ASSETS_DIR "/textures/forest_slope_2k.hdr"
  );

  // Scene Hierarchy.
  {
    focus_ = scene_.import_model( ASSETS_DIR "/models/InfiniteScan/Head.glb" );
  }
  
  // Set renderer parameters.
  {
    auto &params{ renderer_.params() };
    params.show_skybox = true;
    params.show_grid = true;
    params.enable_hair = false;
    params.enable_particle = false;
  }

  //---------------------------

  bRefocus_ = false;
}

void Application::update() {
  auto const& selected = scene_.selected();

  // Refocus the camera when asked for.
  if (bRefocus_) {
    refocus_camera( /*kCentroid*/ true, /*kSmooth*/ false, focus_);
    bRefocus_ = false;
  } else {
    focus_ = selected.empty() ? nullptr : 
                       focus_ ? focus_ : selected.front();
  }

  // -----------------------

  // [Bug-prone due to SceneHierarchy internal structure update, to check]

  // Modify the SceneHierarchy.
  if constexpr(true) {
    // Notes: Adding new entities should be done separately of scene hierarchy modification
    //        as it has not update its internal structure yet.

    auto const& events = Events::Get(); //
    
    if (!selected.empty()) {
      // Reset transform.
      if ('x' == events.lastInputChar()) {
        for (auto &e : selected) {
          scene_.reset_entity(e);
        }
      } 
      // Delete.
      else if ('X' == events.lastInputChar()) {
        for (auto &e : selected) {
          scene_.remove_entity(e, true);
          // focus_ = (focus_ == e) ? nullptr : focus_;
        }
        scene_.select_all(false);
      }
    }

    // Import drag-n-dropped objects & center them to camera target.
    //constexpr float kDragNDropDistance = 2.0f;
    auto const dnd_target = camera_.target(); //camera.position() + kDragNDropDistance * camera.direction();
    for (auto &fn : events.droppedFilenames()) {
      if (auto const ext = fn.substr(fn.find_last_of(".") + 1); MeshDataManager::CheckExtension(ext)) {
        if (auto entity = scene_.import_model(fn); entity) {
          entity->set_position( dnd_target );
        }
      }
    }
  }

  // -----------------------
}

void Application::onInputChar(uint16_t inputChar) {
  auto const& selected = scene_.selected();

  constexpr bool kCentroid{true};
  constexpr bool kSmooth{true};

  // -- Key bindings.
  switch (inputChar) {
    // Select  / Unselect all.
    case 'a':
      scene_.select_all(selected.empty());
    break;
    
    // Focus on entities.
    case 'C':
      refocus_camera( !kCentroid, kSmooth);
    break;
    case 'c':
      refocus_camera( kCentroid, kSmooth);
    break;
    case GLFW_KEY_1:
    case GLFW_KEY_3:
    case GLFW_KEY_7:
      refocus_camera( kCentroid, !kSmooth);
    break;

    // Cycle through entities.
    case 'j':
      refocus_camera( !kCentroid, kSmooth, 
        selected.front() ? scene_.next(selected.front(), +1) : scene_.first()
      );
    break;
    case 'k':
      refocus_camera( !kCentroid, kSmooth, 
        selected.front() ? scene_.next(selected.front(), -1) : scene_.first()
      );
    break;

    // Show / hide UI.
    case 'h':
      App::params().show_ui ^= true;
    break;

    // Show / hide wireframe.
    case 'w':
      renderer_.params().show_wireframe ^= true;
    break;

    default:
    break;
  } 
}

void Application::onResize(int w, int h) {
  LOG_MESSAGE("Resize :", w, h);
  gx::Viewport( w, h);
}

// ----------------------------------------------------------------------------

void Application::refocus_camera(bool bCentroid, bool bSmooth, EntityHandle new_focus) {
  constexpr float kDefaultRefocusDistance{3.5f};
  constexpr float kRefocusDistanceScaling{1.25f};

  // FIXME : issue due to scene reindexing ordering !

  // Select new focus & retrieve target position.
  glm::vec3 target{};
  if (new_focus && new_focus->indexed()) {
    focus_ = new_focus;
    scene_.select_all(false);
    scene_.select(focus_, true);
    target = bCentroid ? scene_.entity_global_centroid(focus_) 
                       : scene_.entity_global_position(focus_)
                       ;
  } else {
    target = bCentroid ? scene_.centroid() : scene_.pivot();
  }

  // Set camera view controller target.
  arcball_controller_.set_target(target, bSmooth);
  
  // Distance to focus point.
  float const radius{ 
    (focus_ && focus_->has<VisualComponent>()) ? focus_->get<VisualComponent>().mesh()->radius() 
                                               : kDefaultRefocusDistance
  };
  arcball_controller_.set_dolly(kRefocusDistanceScaling * radius, bSmooth);
}

// ----------------------------------------------------------------------------
