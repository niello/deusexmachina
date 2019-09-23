#include "DataScheme.h"
#include <Utils.h>

namespace Data
{

bool CDataScheme::Init(const CParams& Desc)
{
	for (const auto& Prm : Desc)
	{
		if (!Prm.second.IsA<CParams>()) return false;
		const CParams& Value = Prm.second.GetValue<CParams>();
		
		CRecord Rec;

		Rec.ID = Prm.first;

		Rec.WriteKey = GetParam<bool>(Value, CStrID("WriteKey"), false);
		Rec.WriteCount = GetParam<bool>(Value, CStrID("WriteCount"), true);
		Rec.WriteChildKeys = GetParam<bool>(Value, CStrID("WriteChildKeys"), false);
		Rec.WriteChildCount = GetParam<bool>(Value, CStrID("WriteChildCount"), true);
		Rec.ApplySchemeToSelf = GetParam<bool>(Value, CStrID("ApplySchemeToSelf"), false);

		Rec.FourCC = 0;
		auto It = Value.find(CStrID("FourCC"));
		if (It != Value.cend()) 
		{
			const auto& FourCC = It->second.GetValue<std::string>();
			if (FourCC.size() == 4)
			{
				Rec.FourCC = FourCC[3] | (FourCC[2] << 8) | (FourCC[1] << 16) | (FourCC[0] << 24);
			}
		}

		It = Value.find(CStrID("Type"));
		if (It != Value.cend())
		{
			const CData* TypeIDVal = &It->second;
			if (TypeIDVal->IsA<int>()) Rec.TypeID = *TypeIDVal;
			else if (TypeIDVal->IsA<std::string>())
			{
				const char* pTypeString = TypeIDVal->GetValue<std::string>().c_str();

				//???move somewhere as common? or even store map of string-to-ID
				if (!_stricmp(pTypeString, "bool")) Rec.TypeID = DATA_TYPE_ID(bool);
				else if (!_stricmp(pTypeString, "int")) Rec.TypeID = DATA_TYPE_ID(int);
				else if (!_stricmp(pTypeString, "float")) Rec.TypeID = DATA_TYPE_ID(float);
				else if (!_stricmp(pTypeString, "string")) Rec.TypeID = DATA_TYPE_ID(std::string);
				else if (!_stricmp(pTypeString, "strid")) Rec.TypeID = DATA_TYPE_ID(CStrID);
				//else if (!_stricmp(pTypeString, "vector3")) Rec.TypeID = DATA_TYPE_ID(vector3);
				//else if (!_stricmp(pTypeString, "vector4")) Rec.TypeID = DATA_TYPE_ID(vector4);
				//else if (!_stricmp(pTypeString, "matrix")) Rec.TypeID = DATA_TYPE_ID(matrix44);
				else Rec.TypeID = -1;
			}
			else assert(false && "CDataScheme::Init -> Wrong type of TypeID param. Must be int or string.");
		}
		else Rec.TypeID = -1;

		It = Value.find(CStrID("Scheme"));
		if (It != Value.cend())
		{
			const CData* SchemeVal = &It->second;
			if (SchemeVal->IsA<CParams>())
			{
				Rec.Scheme.reset(new CDataScheme());
				if (!Rec.Scheme->Init(SchemeVal->GetValue<CParams>())) return false;
			}
			else if (SchemeVal->IsA<CStrID>()) Rec.SchemeID = SchemeVal->GetValue<CStrID>();
			else assert(false && "CDataScheme::Init -> Wrong type of Scheme param. Must be params or strid.");
		}

		It = Value.find(CStrID("Default"));
		if (It != Value.cend())
			Rec.Default = It->second;

		Records.push_back(std::move(Rec));
	}

	return true;
}
//---------------------------------------------------------------------

}
