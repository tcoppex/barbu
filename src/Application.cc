#include "Application.h"

#include "memory/assets/assets.h"
#include "ui/views/views.h"

// ----------------------------------------------------------------------------

// TODO clean usage :
//    * no inherited fields.
//    * no boolean without clear intents.
//    * no weird indirection.

// ----------------------------------------------------------------------------

void Application::setup() {
  // Gamma-corrected clear color.
  gx::ClearColor(0.073f, 0.073f, 0.072f, true);
  //setBackground( Color::kDarkSalmon.gamma() );

  // Camera.
  {
    //auto &camera{ getDefaultCamera() };
    camera_.setController(&arcball_);
    camera_.setPerspective(glm::radians(60.0f), resolution(), 0.01f, 500.0f);
    
    arcball_.setView( kPi / 16.0f, kPi / 8.0f, !kSmooth);
    arcball_.setDolly( 15.0f, !kSmooth);
  }

  // Scene Hierarchy.
  {
    //auto &scene{ getSceneHierarchy() };
    focus_ = scene_.importModel( ResourceId::fromPath("models/InfiniteScan/Head.glb") );
    scene_.createEntity<BSphereEntity>( 0.25f );
    
    bRefocus_ = true;
  }
  
  // Renderer.
  {
    auto &params{ getRendererParameters() };
    params.show_skybox      = true;
    params.show_grid        = true;
    params.enable_hair      = true;
    params.enable_particle  = false;

    // -- Experimentals (probables futures components) --

    // Skybox.
    auto &skybox{ renderer_.skybox() };
    skybox.setup( ResourceId::fromPath("textures/forest_slope_2k.hdr") );
   
    // Hair.
    auto &hair{ renderer_.hair() };
    hair.setup( ResourceId::fromPath("models/InfiniteScan/Head_scalp.obj") );
  }
}

void Application::update() {
  auto const& selected = scene_.selected();

  // Refocus the camera when asked for.
  if (bRefocus_) {
    refocusCamera( kCentroid, !kSmooth, focus_);
    bRefocus_ = false;
  } else {
    focus_ = selected.empty() ? nullptr : (focus_ ? focus_ : selected.front());
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
          scene_.resetEntity(e);
        }
      } 
      // Delete.
      else if ('X' == events.lastInputChar()) {
        for (auto &e : selected) {
          focus_ = (focus_ == e) ? nullptr : focus_;
          scene_.removeEntity(e, true);
        }
        scene_.selectAll(false);
      }
    }

    // Import drag-n-dropped objects & center them to camera target.
    
    auto const get_extension = [](auto path) { 
      return path.substr(path.find_last_of(".") + 1);
    }; 
    auto const dnd_target = camera_.target();

    for (auto &fn : events.droppedFilenames()) {
      if (auto const ext{get_extension(fn)}; MeshDataManager::CheckExtension(ext)) {
        if (auto entity = scene_.importModel(fn); entity) {
          entity->setPosition( dnd_target );
        }
      }
    }
  }
}

void Application::draw() {
  // TODO : 
  // Direct drawing methods without having to use the scene hierarchy.
  //
  // Example :
  //
  // // Custom shader.
  // pgm.load( vsPath, fsPath);
  // pgm.begin();
  //   pgm.setUniform1f( "uValue", val);
  //   drawCircle( x, y, radius);
  // pgm.end();
  //
  // // Default shader.
  // setColor( Color4f(0.5, 1.0, 0.0) );
  // drawRectangle( x, y, w, h);
  //
}

void Application::onInputChar(uint16_t inputChar) {
  auto const& selected = scene_.selected();

  // -- Key bindings.
  switch (inputChar) {
    // Select  / Unselect all.
    case 'a':
      scene_.selectAll(selected.empty());
    break;
    
    // Focus on entities.
    case 'C':
      refocusCamera( !kCentroid, kSmooth);
    break;
    case 'c':
      refocusCamera( kCentroid, kSmooth);
    break;

    // Cycle through entities.
    case 'j':
      refocusCamera( !kCentroid, kSmooth, 
        selected.front() ? scene_.next(selected.front(), +1) : scene_.first()
      );
    break;
    case 'k':
      refocusCamera( !kCentroid, kSmooth, 
        selected.front() ? scene_.next(selected.front(), -1) : scene_.first()
      );
    break;

    // Show / hide UI.
    case 'h':
      App::toggleUI();
    break;

    // Show / hide wireframe.
    case 'w':
      renderer_.params().show_wireframe ^= true; //
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

void Application::refocusCamera(bool _bCentroid, bool _bSmooth, EntityHandle _focus) {
  // FIXME : issue when called before the scene reindexing.

  // Select new focus & retrieve target position.
  glm::vec3 target{};
  if (_focus && _focus->indexed()) {
    focus_ = _focus;
    scene_.selectAll(false);
    scene_.select(focus_, true);
    target = _bCentroid ? scene_.globalCentroid(focus_) 
                       : scene_.globalPosition(focus_)
                       ;
  } else {
    target = _bCentroid ? scene_.centroid() : scene_.pivot();
  }

  // Set the view target.
  arcball_.setTarget(target, _bSmooth);
  
  // Distance to focus point.
  float const focus_dist{
    kRefocusDistanceScaling *  
    ((focus_ && focus_->has<VisualComponent>()) ? focus_->get<VisualComponent>().mesh()->radius() 
                                                : kDefaultRefocusDistance)
  };
  arcball_.setDolly(focus_dist, _bSmooth);
}

// ----------------------------------------------------------------------------
