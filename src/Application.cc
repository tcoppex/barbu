#include "Application.h"

#include "memory/assets/assets.h"
#include "ui/views/views.h"

// ----------------------------------------------------------------------------

void Application::setup() {
  // Gamma-corrected clear color.
  gx::ClearColor(0.145f, true);

  // Camera.
  {
    auto const rx = glm::pi<float>() / 16.0f;
    auto const ry = glm::pi<float>() / 8.0f;
    arcball_controller_.set_view( rx, ry, true);
    arcball_controller_.set_dolly(5.75f);

    auto const& res = resolution();
    camera_.set_controller(&arcball_controller_);
    camera_.set_perspective(glm::radians(60.0f), res.x, res.y, 0.01f, 500.0f);
  }

  // Skybox texture.
  renderer_.skybox().setup_texture(
    ASSETS_DIR "/textures/cross_hdr/uffizi_cross_mmp_s.hdr"
    // ASSETS_DIR "/textures/reinforced_concrete_02_1k.hdr"
  );
  //renderer_.params().show_skybox = false;

  // Scene Hierarchy.
  if constexpr(true) {
    if constexpr (true) {
      // -- Hair sample --

      focus_ = scene_.import_model( ASSETS_DIR "/models/InfiniteScan/Head.glb" );
      renderer_.hair().setup( ASSETS_DIR "/models/InfiniteScan/Head_scalp.obj" ); //
      scene_.add_bounding_sphere(0.25f);
    } else {
      // -- Simple model sample --

      focus_ = scene_.import_model( 
        // ASSETS_DIR "/models/gltf_samples/MetalRoughSpheres/MetalRoughSpheres.gltf"
        // ASSETS_DIR "/models/gltf_samples/CesiumMan.glb"
        // ASSETS_DIR "/models/gltf_samples/DamagedHelmet.glb"
        // ASSETS_DIR "/models/glb-heads/DigitalIra.glb"
        ASSETS_DIR "/models/sintel/sintel_blendshapes.glb" 
      );
    }
  }
  bRefocus_ = true;

  // Recenter the view on the focus centroid.
  //refocus_camera( focus_, /*FocusOnCentroid*/true, /*NoTransition*/false);
}

void Application::update() {
  auto const& eventData{ GetEventData() };
  auto const& selected = scene_.selected();  


  // -- Key bindings.
  switch (eventData.lastChar) {
    // Select  / Unselect all.
    case 'a':
      scene_.select_all(selected.empty());
    break;
    
    // Focus on entities.
    case 'C':
      refocus_camera( false, false);
    break;
    case 'c':
      refocus_camera( true, false);
    break;
    case GLFW_KEY_1:
    case GLFW_KEY_3:
    case GLFW_KEY_7:
      refocus_camera( true, true);

    break;

    // Cycle through entities.
    case 'j':
      refocus_camera( false, false, 
        selected.front() ? scene_.next(selected.front(), +1) : scene_.first()
      );
    break;
    case 'k':
      refocus_camera( false, false, 
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

  if (bRefocus_) {
    refocus_camera(true, true, focus_);
    bRefocus_ = false;
  } else {
    focus_ = selected.empty() ? nullptr : focus_ ? focus_ : selected.front();
  }

  // --------------------------

  // Modify the SceneHierarchy.
  if constexpr(true) {
    // Notes: Adding new entities should be done separately of scene hierarchy modification
    //        as it has not update its internal structure yet.

    if (!selected.empty()) {
      // Reset transform.
      if ('x' == eventData.lastChar) {
        for (auto &e : selected) {
          scene_.reset_entity(e);
        }
      } 
      // Delete.
      else if ('X' == eventData.lastChar) {
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
    for (auto &fn : eventData.dragFilenames) {
      auto const ext = fn.substr(fn.find_last_of(".") + 1);
      if (MeshDataManager::CheckExtension(ext)) {
        if (auto e = scene_.import_model(fn); e) {
          e->set_position( dnd_target );
        }
      }
    }
  }
}

void Application::refocus_camera(bool bCentroid, bool bNoSmooth, EntityHandle new_focus) {
  glm::vec3 target;
  float radius = 3.5f;

  // FIXME : issue due to scene reindexing ordering !

  // Select new focus & retrieve target position.
  if (new_focus && new_focus->indexed()) {
    focus_ = new_focus;
    scene_.select_all(false);
    scene_.select(new_focus, true);
    target = bCentroid ? scene_.entity_global_centroid(new_focus) 
                       : scene_.entity_global_position(new_focus)
                       ;
  } else {
    target = bCentroid ? scene_.centroid() : scene_.pivot();
  }

  // Set camera view controller target.
  arcball_controller_.set_target(target, bNoSmooth);
  
  // Distance to focus point.
  if (focus_ && focus_->has<VisualComponent>()) {
    // radius should be scale with entity global scale.
    auto visual = focus_->get<VisualComponent>();
    radius = visual.mesh()->radius();
  }
  arcball_controller_.set_dolly(1.25f * radius, bNoSmooth);
}

// ----------------------------------------------------------------------------
