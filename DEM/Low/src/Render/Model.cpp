#include "Model.h"
#include <Render/Mesh.h>
#include <Render/Material.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CModel, 'MODL', Render::IRenderable);
}