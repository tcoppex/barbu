#include "Application.h"

#include "memory/assets/assets.h"

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
    //ASSETS_DIR "/textures/reinforced_concrete_02_1k.hdr"
  );
  //renderer_.params().show_skybox = false;

  // Scene Hierarchy.
  EntityHandle e(nullptr);
  if constexpr (true) {
    // -- Hair sample --

    e = scene_.import_model( ASSETS_DIR "/models/InfiniteScan/Head.glb" );
    renderer_.hair().setup( ASSETS_DIR "/models/InfiniteScan/Head_scalp.obj" ); //
    scene_.add_bounding_sphere(0.25f);
  } else {
    // -- Simple model sample --

    e =
    // scene_.import_model( ASSETS_DIR "/models/gltf_samples/MetalRoughSpheres/MetalRoughSpheres.gltf" );
    // scene_.import_model( ASSETS_DIR "/models/gltf_samples/CesiumMan.glb" );
    // scene_.import_model( ASSETS_DIR "/models/gltf_samples/DamagedHelmet.glb" );
    scene_.import_model( ASSETS_DIR "/models/glb-heads/DigitalIra.glb" );
  }

  // Recenter the view on the focus centroid.
  auto const centroid = e ? scene_.entity_global_centroid(e) : scene_.centroid();
  arcball_controller_.set_target( centroid, true);
}

void Application::update() {
  auto const& eventData{ GetEventData() }; //
  auto const& selected = scene_.selected();  
  EntityHandle focus = nullptr;

  // -- Key bindings.
  switch (eventData.lastChar) {
    // Select  / Unselect all.
    case 'a':
      scene_.select_all(selected.empty());
    break;
    
    // Focus on entities.
    case 'C':
      arcball_controller_.set_target(scene_.pivot());
    break;
    case 'c':
      arcball_controller_.set_target(scene_.centroid());
    break;
    case GLFW_KEY_1:
    case GLFW_KEY_3:
    case GLFW_KEY_7:
      arcball_controller_.set_target(scene_.centroid(), true);
    break;

    // Cycle through entities.
    case 'j':
      focus = selected.front() ? scene_.next(selected.front(), +1) : scene_.all().front();
    break;
    case 'k':
      focus = selected.front() ? scene_.next(selected.front(), -1) : scene_.all().front();
    break;

    // Show / hide UI.
    case 'u':
      App::params().show_ui ^= true;
    break;

    // Show / hide wireframe.
    case 'w':
      renderer_.params().show_wireframe ^= true;
    break;

    default:
    break;
  }

  // Recenter camera on focus.
  if (focus) {
    scene_.select_all(false);
    scene_.select(focus, true);
    auto const& target = scene_.entity_global_position(focus);
    arcball_controller_.set_target(target); 
  }

  // -- Key bindings on selection.
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

// ----------------------------------------------------------------------------
