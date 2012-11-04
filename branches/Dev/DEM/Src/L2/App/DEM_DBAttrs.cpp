#include "Environment.h"

namespace Attr
{
	DeclareAttrsModule(StdAttrs)
	DeclareAttrsModule(Database)
	DeclareAttrsModule(PropTransformable)
	DeclareAttrsModule(PropCamera)
	DeclareAttrsModule(PropChaseCamera)
	DeclareAttrsModule(PropCharGraphics)
	DeclareAttrsModule(TimeServer)
	DeclareAttrsModule(LoaderServer)
	DeclareAttrsModule(FocusManager)
	DeclareAttrsModule(PropAbstractGraphics)
	DeclareAttrsModule(PropPhysics)
	DeclareAttrsModule(PropLight)
	DeclareAttrsModule(PropTime)
	DeclareAttrsModule(PropVideoCamera)
	DeclareAttrsModule(PropVideoCamera2)
	DeclareAttrsModule(PropPathAnim)

	// New attrs
	DeclareAttrsModule(ScriptObject)
	DeclareAttrsModule(PropSceneNode)
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
	RegisterAttrs(PropCharGraphics)
	RegisterAttrs(TimeServer)
	RegisterAttrs(LoaderServer)
	RegisterAttrs(FocusManager)
	RegisterAttrs(PropAbstractGraphics)
	RegisterAttrs(PropPhysics)
	RegisterAttrs(PropLight)
	RegisterAttrs(PropTime)
	RegisterAttrs(PropVideoCamera)
	RegisterAttrs(PropVideoCamera2)
	RegisterAttrs(PropPathAnim)

	// New attrs
	RegisterAttrs(ScriptObject)
	RegisterAttrs(PropSceneNode)
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