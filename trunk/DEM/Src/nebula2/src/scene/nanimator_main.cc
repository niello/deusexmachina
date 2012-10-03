//------------------------------------------------------------------------------
//  nanimator_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nanimator.h"
#include "variable/nvariableserver.h"
#include <kernel/nkernelserver.h>
#include <Data/BinaryReader.h>

nNebulaClass(nAnimator, "nscenenode");

//------------------------------------------------------------------------------
/**
*/
nAnimator::nAnimator() :
    loopType(nAnimLoopType::Loop)
{
    this->channelVarHandle = nVariableServer::Instance()->GetVariableHandleByName("time");
    this->channelOffsetVarHandle = nVariableServer::Instance()->GetVariableHandleByName("timeOffset");
}

bool nAnimator::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'LNHC': // CHNL
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			SetChannel(Value);
			OK;
		}
		case 'PTPL': // LPTP
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			SetLoopType(nAnimLoopType::FromString(Value));
			OK;
		}
		default: return nSceneNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Returns the type of the animator object. Subclasses should return
    something meaningful here.
*/
nAnimator::Type
nAnimator::GetAnimatorType() const
{
    return InvalidType;
}

//------------------------------------------------------------------------------
/**
    This method is called back by scene node objects which wish to be
    animated.
*/
void
nAnimator::Animate(nSceneNode* /*sceneNode*/, nRenderContext* /*renderContext*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Sets the "animation channel" which drives this animation.
    This could be something like "time", but the actual names are totally
    up to the application. The actual channel value will be pulled from
    the render context provided in the Animate() method.
*/
void
nAnimator::SetChannel(const char* name)
{
    n_assert(name);
    this->channelVarHandle = nVariableServer::Instance()->GetVariableHandleByName(name);
    this->channelOffsetVarHandle = nVariableServer::Instance()->GetVariableHandleByName((nString(name) + "Offset").Get());
}

//------------------------------------------------------------------------------
/**
    Return the animation channel which drives this animation.
*/
const char*
nAnimator::GetChannel()
{
    if (nVariable::InvalidHandle == this->channelVarHandle)
    {
        return 0;
    }
    else
    {
        return nVariableServer::Instance()->GetVariableName(this->channelVarHandle);
    }
}

