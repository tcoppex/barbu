#ifndef BARBU_UI_VIEWS_MAIN_H_
#define BARBU_UI_VIEWS_MAIN_H_

#include <vector>

#include "ui/ui_view.h"
#include "core/app.h"

namespace views {

class Main : public ParametrizedUIView<App::Parameters_t> {
 public:
  Main(TParameters &params) : ParametrizedUIView(params) {}

  void render() final;
  
  inline void push_view(UIView* view) {
    views_.push_back(view);
  }

private:
  std::vector<UIView*> views_;
};

}  // namespace views

#endif  // BARBU_UI_VIEWS_MAIN_H_
