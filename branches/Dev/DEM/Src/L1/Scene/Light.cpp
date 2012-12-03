#include "Light.h"

#include <Scene/Scene.h>
#include <Data/BinaryReader.h>

namespace Scene
{
ImplementRTTI(Scene::CLight, Scene::CSceneNodeAttr);
ImplementFactory(Scene::CLight);

bool CLight::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
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

				//if (Type == DATA_TYPE_ID(bool)) SetBool(Param, DataReader.Read<bool>());
				//else if (Type == DATA_TYPE_ID(int)) SetInt(Param, DataReader.Read<int>());
				//else if (Type == DATA_TYPE_ID(float)) SetFloat(Param, DataReader.Read<float>());
				//else if (Type == DATA_TYPE_ID(vector4)) SetVector(Param, DataReader.Read<vector4>()); //???vector3?
				////else if (Type == DATA_TYPE_ID(matrix44)) SetMatrix(Param, DataReader.Read<matrix44>());
				//else FAIL;
				if (Type == DATA_TYPE_ID(bool)) DataReader.Read<bool>();
				else if (Type == DATA_TYPE_ID(int)) DataReader.Read<int>();
				else if (Type == DATA_TYPE_ID(float)) DataReader.Read<float>();
				else if (Type == DATA_TYPE_ID(vector4)) DataReader.Read<vector4>();
				else FAIL;
			}
			OK;
		}
		case 'THGL': // LGHT
		{
			DataReader.Read<int>((int&)Type); // To force size
			OK;
		}
		case 'DHSC': // CSHD
		{
			//!!!Flags.SetTo(ShadowCaster, DataReader.Read<bool>());!
			DataReader.Read<bool>();
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CLight::Update(CScene& Scene)
{
	if (Type == Directional) Scene.AddVisibleLight(*this);
	else
	{
		if (!pSPSRecord)
		{
			CSPSRecord NewRec;
			NewRec.pAttr = this;
			GetBox(NewRec.GlobalBox);
			pSPSRecord = Scene.SPS.AddObject(NewRec);

			//!!!on delete attr with valid SPS handle, remove it from SPS!
		}
		else
		{
			//!!!only if local box or global tfm changed!
			GetBox(pSPSRecord->GlobalBox);
			Scene.SPS.UpdateElement(pSPSRecord);
		}
	}
}
//---------------------------------------------------------------------

}