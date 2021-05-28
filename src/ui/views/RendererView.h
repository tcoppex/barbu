#ifndef BARBU_UI_VIEWS_RENDERER_VIEW_H_
#define BARBU_UI_VIEWS_RENDERER_VIEW_H_

#include "ui/ui_view.h"
#include "core/renderer.h"

namespace views {

class RendererView : public ParametrizedUIView<Renderer::Parameters_t> {
 public:
  RendererView(TParameters &params) : ParametrizedUIView(params) {}

  void render() final;
};

}  // namespace views

#endif  // BARBU_UI_VIEWS_RENDERER_VIEW_H_
