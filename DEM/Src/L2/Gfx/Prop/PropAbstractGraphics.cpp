#include "PropAbstractGraphics.h"

#include <Game/Entity.h>
#include <DB/DBServer.h>

namespace Attr
{
	DefineString(Graphics);
}

BEGIN_ATTRS_REGISTRATION(PropAbstractGraphics)
	RegisterString(Graphics, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropAbstractGraphics, Game::CProperty);
ImplementFactory(Properties::CPropAbstractGraphics);

// Get the default graphics resource, which is Attr::Graphics.
// subclasses may override this to provide a self managed resource.
nString CPropAbstractGraphics::GetGraphicsResource()
{
	return GetEntity()->Get<nString>(Attr::Graphics);
}
//---------------------------------------------------------------------

} // namespace Properties
