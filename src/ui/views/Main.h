#ifndef BARBU_UI_VIEWS_MAIN_H_
#define BARBU_UI_VIEWS_MAIN_H_

#include <vector>
#include <memory>

#include "ui/ui_view.h"
#include "core/app.h"

namespace views {

class Main : public ParametrizedUIView<App::Parameters_t> {
 public:
  //using UIViewHandle = std::shared_ptr<UIView>;

  Main(TParameters &params)
    : ParametrizedUIView(params)
  {}

  void render() final;
  
  inline void push_view(std::shared_ptr<UIView> view) {
    views_.push_back(view);
  }

private:
  std::vector<std::shared_ptr<UIView>> views_;
};

}  // namespace "views"

#endif  // BARBU_UI_VIEWS_MAIN_H_
