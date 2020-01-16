#include "NodeAttribute.h"
#include <Scene/SceneNode.h>

namespace Scene
{
RTTI_CLASS_IMPL(Scene::CNodeAttribute, Core::CObject);

void CNodeAttribute::RemoveFromNode()
{
	if (_pNode) _pNode->RemoveAttribute(*this);
}
//---------------------------------------------------------------------

void CNodeAttribute::UpdateActivity()
{
	const bool WasActive = IsActive();
	const bool NowActive = IsActiveSelf() && _pNode && _pNode->IsActive();
	if (WasActive != NowActive)
	{
		_Flags.SetTo(EffectivelyActive, NowActive);
		OnActivityChanged(NowActive);
	}
}
//---------------------------------------------------------------------

}