#include <cstdlib>
#include "core/app.h"
#include "scene.h"

// ----------------------------------------------------------------------------

#define WINDOW_TITLE    "- barbü -"

// ----------------------------------------------------------------------------

int main(int, char *[]) {
  App app;
  Scene scene;

  if (!app.init(WINDOW_TITLE, &scene)) {
    return EXIT_FAILURE;
  }
  app.run();
  app.deinit();

  return EXIT_SUCCESS;
}

// ----------------------------------------------------------------------------
