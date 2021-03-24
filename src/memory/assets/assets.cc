#include "memory/assets/assets.h"

// ----------------------------------------------------------------------------

#define DEFINE_FACTORY(name) \
  name##Factory Assets::s##name ; 

DEFINE_FACTORY(Texture)
DEFINE_FACTORY(Mesh)
DEFINE_FACTORY(Program)
DEFINE_FACTORY(MaterialAsset)

// ----------------------------------------------------------------------------