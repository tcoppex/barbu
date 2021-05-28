#include "Application.h"

// ----------------------------------------------------------------------------

#ifndef WINDOW_TITLE
  #define WINDOW_TITLE    "- barbü -"
#endif

int main(int, char *[]) {
  return Application().run( WINDOW_TITLE );
}

// ----------------------------------------------------------------------------
