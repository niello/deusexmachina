#include "scene/nlightnode.h"
#include "gfx2/ngfxserver2.h"
#include "gfx2/nshader2.h"
#include <Data/BinaryReader.h>

nNebulaClass(nLightNode, "ntransformnode");

bool nLightNode::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'THGL': // LGHT
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			Light.SetType(nLight::StringToType(Value));
			OK;
		}
		case 'DHSC': // CSHD
		{
			Light.SetCastShadows(DataReader.Read<bool>());
			OK;
		}
		default: return nTransformNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------
