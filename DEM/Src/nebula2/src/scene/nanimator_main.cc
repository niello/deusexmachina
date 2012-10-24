#include "scene/nanimator.h"
#include "variable/nvariableserver.h"
#include <kernel/nkernelserver.h>
#include <Data/BinaryReader.h>

nNebulaClass(nAnimator, "nscenenode");

nAnimator::nAnimator(): LoopType(nAnimLoopType::Loop)
{
	HChannel = nVariableServer::Instance()->GetVariableHandleByName("time");
	HChannelOffset = nVariableServer::Instance()->GetVariableHandleByName("timeOffset");
}
//---------------------------------------------------------------------

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
			LoopType = nAnimLoopType::FromString(Value);
			OK;
		}
		default: return nSceneNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Sets the "animation channel" which drives this animation.
    This could be something like "time", but the actual names are totally
    up to the application. The actual channel value will be pulled from
    the render context provided in the Animate() method.
*/
void nAnimator::SetChannel(const char* name)
{
    n_assert(name);
    HChannel = nVariableServer::Instance()->GetVariableHandleByName(name);
    HChannelOffset = nVariableServer::Instance()->GetVariableHandleByName((nString(name) + "Offset").Get());
}
//---------------------------------------------------------------------

// Return the animation channel which drives this animation.
const char* nAnimator::GetChannel()
{
	return HChannel == nVariable::InvalidHandle ? NULL : nVariableServer::Instance()->GetVariableName(HChannel);
}
//---------------------------------------------------------------------
