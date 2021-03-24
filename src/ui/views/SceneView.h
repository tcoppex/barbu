#ifndef BARBU_UI_VIEWS_SCENE_H_
#define BARBU_UI_VIEWS_SCENE_H_

#include "ui/ui_view.h"
#include "scene.h"

namespace views {

class SceneView : public ParametrizedUIView<Scene::Parameters_t> {
 public:
  SceneView(TParameters &params) : ParametrizedUIView(params) {}

  void render() final;
};

}  // namespace views

#endif  // BARBU_UI_VIEWS_SCENE_H_
