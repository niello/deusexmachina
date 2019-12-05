#include "Skybox.h"
#include <Render/Material.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CSkybox, 'SKBX', Render::IRenderable);

PRenderable CSkybox::Clone()
{
	CSkybox* pCloned = n_new(CSkybox);
	pCloned->Material = Material;
	pCloned->Mesh = Mesh;
	return PRenderable(pCloned);
}
//---------------------------------------------------------------------

}