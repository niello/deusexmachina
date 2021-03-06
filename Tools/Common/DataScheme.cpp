#include "DataScheme.h"
#include <Utils.h>
#include <ParamsUtils.h>

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

		Rec.WriteKey = ParamsUtils::GetParam<bool>(Value, "WriteKey", false);
		Rec.WriteCount = ParamsUtils::GetParam<bool>(Value, "WriteCount", true);
		Rec.WriteChildKeys = ParamsUtils::GetParam<bool>(Value, "WriteChildKeys", false);
		Rec.WriteChildCount = ParamsUtils::GetParam<bool>(Value, "WriteChildCount", true);
		Rec.ApplySchemeToSelf = ParamsUtils::GetParam<bool>(Value, "ApplySchemeToSelf", false);

		Rec.FourCC = 0;

		std::string FourCC;
		if (ParamsUtils::TryGetParam(FourCC, Value, "FourCC"))
		{
			if (FourCC.size() == 4)
			{
				Rec.FourCC = FourCC[3] | (FourCC[2] << 8) | (FourCC[1] << 16) | (FourCC[0] << 24);
			}
		}

		Data::CData TypeIDVal;
		if (ParamsUtils::TryGetParam(TypeIDVal, Value, "Type"))
		{
			if (TypeIDVal.IsA<int>()) Rec.TypeID = TypeIDVal;
			else if (TypeIDVal.IsA<std::string>())
			{
				std::string TypeString = TypeIDVal.GetValue<std::string>();
				ToLower(TypeString);

				//!!!TODO: support unsigned types and bit count (uint64_t etc)!

				//???move somewhere as common? or even store map of string-to-ID
				if (TypeString == "bool") Rec.TypeID = DATA_TYPE_ID(bool);
				else if (TypeString == "int") Rec.TypeID = DATA_TYPE_ID(int);
				else if (TypeString == "float") Rec.TypeID = DATA_TYPE_ID(float);
				else if (TypeString == "string") Rec.TypeID = DATA_TYPE_ID(std::string);
				else if (TypeString == "strid") Rec.TypeID = DATA_TYPE_ID(CStrID);
				else if (TypeString == "vector2")
				{
					Rec.TypeID = DATA_TYPE_ID(float4);
					Rec.ComponentCount = 2;
				}
				else if (TypeString == "vector3")
				{
					Rec.TypeID = DATA_TYPE_ID(float4);
					Rec.ComponentCount = 3;
				}
				else if (TypeString == "vector4")
				{
					Rec.TypeID = DATA_TYPE_ID(float4);
					Rec.ComponentCount = 4;
				}
				//else if (TypeString == "matrix")) Rec.TypeID = DATA_TYPE_ID(matrix44);
				else
				{
					assert(false && "CDataScheme::Init() > Unsupported TypeID");
					Rec.TypeID = -1;
				}
			}
			else assert(false && "CDataScheme::Init() > Wrong type of TypeID param. Must be int or string.");
		}
		else Rec.TypeID = -1;

		Data::CData SchemeVal;
		if (ParamsUtils::TryGetParam(SchemeVal, Value, "Scheme"))
		{
			if (SchemeVal.IsA<CParams>())
			{
				Rec.Scheme.reset(new CDataScheme());
				if (!Rec.Scheme->Init(SchemeVal.GetValue<CParams>())) return false;
			}
			else if (SchemeVal.IsA<CStrID>()) Rec.SchemeID = SchemeVal.GetValue<CStrID>();
			else assert(false && "CDataScheme::Init -> Wrong type of Scheme param. Must be params or strid.");
		}

		ParamsUtils::TryGetParam(Rec.Default, Value, "Default");

		Records.push_back(std::move(Rec));
	}

	return true;
}
//---------------------------------------------------------------------

}
