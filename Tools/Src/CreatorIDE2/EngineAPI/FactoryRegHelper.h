// AI ==============================================

//Props
#include <Animation/PropAnimation.h>
#include <Physics/PropPhysics.h>
#include <AI/PropAIHints.h>

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
#include <Items/ActionTpls/ActionTplEquipItem.h>
#include <Items/ActionTpls/ActionTplPickItemWorld.h>
#include <AI/SmartObj/ActionTpls/ActionTplGotoSmartObj.h>
#include <AI/SmartObj/ActionTpls/ActionTplUseSmartObj.h>

// Actions
#include <AI/Movement/Actions/ActionGoto.h>
#include <AI/Movement/Actions/ActionSteerToPosition.h>
#include <AI/SmartObj/ValidatorScript.h>
#include "AI/SmartObj/ValidatorContainerUIStatus.h"
#include "AI/SmartObj/ValidatorPlrOnly.h"
#include <AI/SmartObj/ValidatorCanTalk.h>
#include <AI/SmartObj/ValidatorDlgRuns.h>
#include <AI/Planning/WorldStateSourceScript.h>

// PROPS ===========================================

#include <Physics/PropTrigger.h>
#include <Scripting/PropScriptable.h>
#include <Combat/Prop/PropDestructible.h>
#include <Items/Prop/PropItem.h>
#include <Items/Prop/PropEquipment.h>

// RENDERERS  ======================================

#include <Render/Renderers/DebugGeomRenderer.h>
#include <Render/Renderers/DebugTextRenderer.h>
#include <Render/Renderers/ModelRenderer.h>
#include <Render/Renderers/TerrainRenderer.h>

// OTHER ===========================================

#include <Physics/CollisionShape.h>
#include <Items/ItemTplWeapon.h>
#include <Debug/LuaConsole.h>
#include <Debug/WatcherWindow.h>
#include <UI/UIRenderer.h>

void ForceFactoryRegistration()
{
	Render::CDebugGeomRenderer::ForceFactoryRegistration();
	Render::CDebugTextRenderer::ForceFactoryRegistration();
	Render::CModelRenderer::ForceFactoryRegistration();
	Render::CTerrainRenderer::ForceFactoryRegistration();
	
	Debug::CLuaConsole::ForceFactoryRegistration();
	Debug::CWatcherWindow::ForceFactoryRegistration();
	Render::CUIRenderer::ForceFactoryRegistration();
}