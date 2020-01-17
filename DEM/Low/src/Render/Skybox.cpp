#include "Skybox.h"
#include <Render/Material.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CSkybox, 'SKBX', Render::IRenderable);
}