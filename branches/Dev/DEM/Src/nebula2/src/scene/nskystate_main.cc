#include "scene/nskystate.h"
#include "gfx2/ngfxserver2.h"
#include <Data/BinaryReader.h>

nNebulaClass(nSkyState, "ntransformnode");

bool nSkyState::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'SRAV': // VARS
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				char Key[256];
				if (!DataReader.ReadString(Key, sizeof(Key))) FAIL;
				nShaderState::Param Param = nShaderState::StringToParam(Key);

				char Type;
				if (!DataReader.Read(Type)) FAIL;

				if (Type == DATA_TYPE_ID(bool)) SetBool(Param, DataReader.Read<bool>());
				else if (Type == DATA_TYPE_ID(int)) SetInt(Param, DataReader.Read<int>());
				else if (Type == DATA_TYPE_ID(float)) SetFloat(Param, DataReader.Read<float>());
				else if (Type == DATA_TYPE_ID(vector4)) SetVector(Param, DataReader.Read<vector4>()); //???vector3?
				//else if (Type == DATA_TYPE_ID(matrix44)) SetMatrix(Param, DataReader.Read<matrix44>());
				else FAIL;
			}
			OK;
		}
		default: return nTransformNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------
