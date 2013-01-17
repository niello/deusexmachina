// AI ==============================================

//Props
#include <AI/Prop/PropActorAnimation.h>
#include <AI/Prop/PropActorPhysics.h>
#include <AI/Prop/PropAIHints.h>
#include <Scene/PropSceneNode.h>

// Sensors, perceptors, stimuli
#include <AI/Sensors/SensorVision.h>
#include <AI/Perceptors/PerceptorObstacle.h>
#include <AI/Perceptors/PerceptorOverseer.h>
#include <AI/Perceptors/PerceptorSmartObj.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <AI/Stimuli/StimulusSound.h>

// Goals, action tpls
#include <AI/Goals/GoalWander.h>
#include <AI/Goals/GoalWork.h>
#include <AI/Planning/ActionTplIdle.h>
#include <AI/ActionTpls/ActionTplWander.h>
#include <Chr/ActionTpls/ActionTplEquipItem.h>
#include <Items/ActionTpls/ActionTplPickItemWorld.h>
#include <AI/SmartObj/ActionTpls/ActionTplGotoSmartObj.h>
#include <AI/SmartObj/ActionTpls/ActionTplUseSmartObj.h>

// Actions
#include <AI/Movement/Actions/ActionGoto.h>
#include <AI/Movement/Actions/ActionSteerToPosition.h>
#include <AI/SmartObj/ValidatorScript.h>
#include <AI/SmartObj/ValidatorContainerUIStatus.h>
#include <AI/SmartObj/ValidatorPlrOnly.h>
#include <AI/SmartObj/ValidatorCanTalk.h>
#include <AI/SmartObj/ValidatorDlgRuns.h>
#include <AI/Planning/WorldStateSourceScript.h>

// PROPS ===========================================

#include <Gfx/Prop/PropCharGraphics.h>
#include <Plr/Prop/PropPlrCharacterInput.h>
#include <Physics/Prop/PropTrigger.h>
#include <Scripting/Prop/PropScriptable.h>
#include <Combat/Prop/PropDestructible.h>
#include <World/Prop/PropTransitionZone.h>
#include <Items/Prop/PropItem.h>
#include <Chr/Prop/PropEquipment.h>

// RENDERERS =======================================

#include <Render/Renderers/ModelRendererNoLight.h>
#include <Render/Renderers/ModelRendererSinglePassLight.h>

// OTHER ===========================================

#include <Physics/Collision/Shape.h>
#include <Items/ItemTplWeapon.h>
#include <Debug/LuaConsole.h>
#include <Debug/WatcherWindow.h>
