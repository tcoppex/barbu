#include "Application.h"

#include "memory/assets/assets.h"
#include "ui/views/views.h"

// ----------------------------------------------------------------------------
//
// TODO clean usage :
//    * [✓] no inherited fields.
//    * [✓] no weird indirections.
//    * [ ] no boolean without clear intents.
//    * [ ] fix scene hierarchy reindexing issue.
//
// ----------------------------------------------------------------------------

void Application::setup() {
  // Gamma-corrected clear color.
  gx::ClearColor(0.25f, true);
  
  // setBackground( Color::kDarkSalmon.gamma() );

  // Renderer.
  {
    auto &renderer{ getRenderer() };
    
    // Parameters.
    auto &params{ renderer.params() };
    params.show_skybox      = true;
    params.show_grid        = true;
    params.enable_hair      = true;
    params.enable_particle  = false;

    // Skybox.
    auto &skybox{ renderer.skybox() };
    skybox.setup( ResourceId::fromPath("textures/forest_slope_2k.hdr") );
   
    // Hair.
    auto &hair{ renderer.hair() };
    hair.setup( ResourceId::fromPath("models/InfiniteScan/Head_scalp.obj") );
  }

  // Camera.
  {
    auto &camera{ getDefaultCamera() };

    camera.setController(&arcball_);
    camera.setPerspective(glm::radians(60.0f), getWindow()->resolution(), 0.01f, 500.0f);

    arcball_.setView(kPi / 16.0f, kPi / 8.0f, !kSmooth);
    arcball_.setDolly(15.0f, !kSmooth);
  }

  // Scene Hierarchy.
  {
    auto &scene{ getSceneHierarchy() };

    focus_ = scene.importModel( ResourceId::fromPath("models/InfiniteScan/Head.glb") );
    scene.createEntity<BSphereEntity>( 0.25f );

    // Refocus on the next update.
    bRefocus_ = true;
  }
}

void Application::update() {
  auto &scene{ getSceneHierarchy() };
  auto const& selected{ scene.selected() };

  // Refocus the camera when asked for.
  if (bRefocus_) {
    refocusCamera( kCentroid, !kSmooth, focus_);
    bRefocus_ = false;
  } else {
    focus_ = selected.empty() ? nullptr : (focus_ ? focus_ : selected.front());
  }

  // Events that modify the SceneHierarchy.
  if constexpr(true) {
    updateHierarchyEvents(); //
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

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------

void Application::onInputChar(uint16_t inputChar) {
  auto &scene{ getSceneHierarchy() };
  auto &rendererParams{ getRenderer().params() };

  auto const& selected{ scene.selected() };
  
  int cycling_step = 0;

  // -- Key bindings.
  switch (inputChar) {
    // Select / Deselect all.
    case 'a':
      selected.empty() ? scene.selectAll() : scene.deselectAll();
    break;
    
    // Focus on current entity.
    case 'C':
      refocusCamera( !kCentroid, kSmooth);
    break;
    case 'c':
      refocusCamera( kCentroid, kSmooth);
    break;

    // Cycle through selected entities.
    case 'j':
      cycling_step = +1;
    break;
    case 'k':
      cycling_step = -1;
    break;

    // Toggle UI display.
    case 'h':
      params().toggleUI();
    break;

    // Toggle Wireframe display.
    case 'w':
      rendererParams.toggleWireframe();
    break;

    default:
    break;
  } 

  if (0 != cycling_step) {
    focus_ = selected.front() ? scene.next(selected.front(), cycling_step) 
                              : scene.first()
                              ;
    refocusCamera( !kCentroid, kSmooth, focus_);
  }
}

void Application::onResize(int w, int h) {
  LOG_MESSAGE("onResize :", w, h);
  gx::Viewport( w, h);
}

// ----------------------------------------------------------------------------

void Application::refocusCamera(bool _bCentroid, bool _bSmooth, EntityHandle _focus) {
  // [ FIXME : issue when called before the scene reindexing. ]

  auto &scene{ getSceneHierarchy() };

  // Select new focus & retrieve target position.
  glm::vec3 target{};
  if (_focus && _focus->indexed()) {
    focus_ = _focus;
    scene.deselectAll();
    scene.select(focus_);
    target = _bCentroid ? scene.globalCentroid(focus_) 
                        : scene.globalPosition(focus_)
                        ;
  } else {
    target = _bCentroid ? scene.centroid() 
                        : scene.pivot()
                        ;
  }

  // Distance to focus point.
  float const focus_distance{
    kRefocusDistanceScaling *  
    ((focus_ && focus_->has<VisualComponent>()) ? focus_->get<VisualComponent>().mesh()->radius() 
                                                : kDefaultRefocusDistance)
  };

  arcball_.setTarget( target, _bSmooth);
  arcball_.setDolly( focus_distance, _bSmooth);
}

void Application::updateHierarchyEvents() {
  //
  // [ Bug-prone due to SceneHierarchy internal structure update, to investigate ]
  //
  // Adding new entities should be done separately of scene hierarchy modification
  // as it has not update its internal structure yet.
  //
  // Depending on where this is called, this might fails. 
  //

  auto const& events{ Events::Get() }; 
  auto &scene{ getSceneHierarchy() };
  
  // Modify selected entities.
  if (auto const& selected{ scene.selected() }; !selected.empty()) {
    switch (events.lastInputChar()) {
      // Reset transform.
      case 'x':
        for (auto &e : selected) {
          scene.resetEntity(e);
        }
      break;

      // Delete.
      case 'X':
        for (auto &e : selected) {
          focus_ = (focus_ == e) ? nullptr : focus_;
          scene.removeEntity(e, true);
        }
        scene.deselectAll();
      break;

      default:
      break;
    };
  }

  // Import drag-n-dropped objects & center them to the camera target.
  auto const get_extension{[](auto path) { 
    return path.substr(path.find_last_of(".") + 1);
  }};
  for (auto &fn : events.droppedFilenames()) {
    if (auto const ext{get_extension(fn)}; MeshDataManager::CheckExtension(ext)) {
      if (auto entity = scene.importModel(fn); entity) {
        entity->setPosition( camera_.target() );
      }
    }
  }
}

// ----------------------------------------------------------------------------
