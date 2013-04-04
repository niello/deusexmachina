#include "Environment.h"

namespace Attr
{
	DeclareAttrsModule(StdAttrs)
	DeclareAttrsModule(Database)
	DeclareAttrsModule(PropTransformable)
	DeclareAttrsModule(PropCamera)
	DeclareAttrsModule(PropChaseCamera)
	DeclareAttrsModule(TimeServer)
	DeclareAttrsModule(LoaderServer)
	DeclareAttrsModule(FocusManager)
	DeclareAttrsModule(PropAbstractGraphics)
	DeclareAttrsModule(PropPhysics)
	DeclareAttrsModule(PropLight)
	DeclareAttrsModule(PropTime)

	// New attrs
	DeclareAttrsModule(ScriptObject)
	DeclareAttrsModule(PropSceneNode)
	DeclareAttrsModule(PropAnimation)
	DeclareAttrsModule(PropUIControl)
	DeclareAttrsModule(PropSmartObject)
	DeclareAttrsModule(PropWeapon)
	DeclareAttrsModule(PropTalking)
	DeclareAttrsModule(PropActorBrain)
	DeclareAttrsModule(PropAIHints)
	DeclareAttrsModule(PropScriptable)
	DeclareAttrsModule(PropTrigger)
	DeclareAttrsModule(PropTransitionZone)
	DeclareAttrsModule(QuestSystem)
	DeclareAttrsModule(ItemAttrs)
};

namespace App
{

void CEnvironment::RegisterAttributes()
{
	RegisterAttrs(StdAttrs)
	RegisterAttrs(Database)
	RegisterAttrs(PropTransformable)
	RegisterAttrs(PropCamera)
	RegisterAttrs(PropChaseCamera)
	RegisterAttrs(TimeServer)
	RegisterAttrs(LoaderServer)
	RegisterAttrs(FocusManager)
	RegisterAttrs(PropAbstractGraphics)
	RegisterAttrs(PropPhysics)
	RegisterAttrs(PropLight)
	RegisterAttrs(PropTime)

	// New attrs
	RegisterAttrs(ScriptObject)
	RegisterAttrs(PropSceneNode)
	RegisterAttrs(PropAnimation)
	RegisterAttrs(PropUIControl)
	RegisterAttrs(PropSmartObject)
	RegisterAttrs(PropWeapon)
	RegisterAttrs(PropTalking)
	RegisterAttrs(PropActorBrain)
	RegisterAttrs(PropAIHints)
	RegisterAttrs(PropScriptable)
	RegisterAttrs(PropTrigger)
	RegisterAttrs(PropTransitionZone)
	RegisterAttrs(QuestSystem)
	RegisterAttrs(ItemAttrs)
}
//---------------------------------------------------------------------

}