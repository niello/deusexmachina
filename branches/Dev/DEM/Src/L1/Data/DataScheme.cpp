#include "DataScheme.h"

#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Math/Matrix44.h>

namespace Data
{

bool CDataScheme::Init(const CParams& Desc)
{
	for (int i = 0; i < Desc.GetCount(); ++i)
	{
		const CParam& Prm = Desc.Get(i);
		if (!Prm.IsA<PParams>()) FAIL;
		const CParams& Value = *Prm.GetValue<PParams>();
		
		CRecord Rec;

		Rec.ID = Prm.GetName();

		Rec.Flags.SetTo(WRITE_KEY, Value.Get<bool>(CStrID("WriteKey"), false));
		Rec.Flags.SetTo(WRITE_COUNT, Value.Get<bool>(CStrID("WriteCount"), true));
		Rec.Flags.SetTo(WRITE_CHILD_KEYS, Value.Get<bool>(CStrID("WriteChildKeys"), false));
		Rec.Flags.SetTo(WRITE_CHILD_COUNT, Value.Get<bool>(CStrID("WriteChildCount"), true));
		Rec.Flags.SetTo(APPLY_SCHEME_TO_SELF, Value.Get<bool>(CStrID("ApplySchemeToSelf"), false));

		CString FourCC;
		if (Value.Get<CString>(FourCC, CStrID("FourCC")))
			Rec.FourCC.FromString(FourCC.CStr());
		else Rec.FourCC = 0;

		CData* TypeIDVal;
		if (Value.Get(TypeIDVal, CStrID("Type")))
		{
			if (TypeIDVal->IsA<int>()) Rec.TypeID = *TypeIDVal;
			else if (TypeIDVal->IsA<CString>())
			{
				LPCSTR pTypeString = TypeIDVal->GetValue<CString>().CStr();

				//???move somewhere as common? or even store map of string-to-ID
				if (!n_stricmp(pTypeString, "bool")) Rec.TypeID = DATA_TYPE_ID(bool);
				else if (!n_stricmp(pTypeString, "int")) Rec.TypeID = DATA_TYPE_ID(int);
				else if (!n_stricmp(pTypeString, "float")) Rec.TypeID = DATA_TYPE_ID(float);
				else if (!n_stricmp(pTypeString, "string")) Rec.TypeID = DATA_TYPE_ID(CString);
				else if (!n_stricmp(pTypeString, "strid")) Rec.TypeID = DATA_TYPE_ID(CStrID);
				else if (!n_stricmp(pTypeString, "vector3")) Rec.TypeID = DATA_TYPE_ID(vector3);
				else if (!n_stricmp(pTypeString, "vector4")) Rec.TypeID = DATA_TYPE_ID(vector4);
				else if (!n_stricmp(pTypeString, "matrix")) Rec.TypeID = DATA_TYPE_ID(matrix44);
				else Rec.TypeID = -1;
			}
			else Sys::Error("CDataScheme::Init -> Wrong type of TypeID param. Must be int or string.");
		}
		else Rec.TypeID = -1;

		CData* SchemeVal;
		if (Value.Get(SchemeVal, CStrID("Scheme")))
		{
			if (SchemeVal->IsA<PParams>())
			{
				Rec.Scheme = n_new(CDataScheme);
				if (!Rec.Scheme->Init(*SchemeVal->GetValue<PParams>())) FAIL;
			}
			else if (SchemeVal->IsA<CStrID>()) Rec.SchemeID = SchemeVal->GetValue<CStrID>();
			else Sys::Error("CDataScheme::Init -> Wrong type of Scheme param. Must be params or strid.");
		}

		CParam* pDflt;
		if (Value.Get(pDflt, CStrID("Default")))
			Rec.Default = pDflt->GetRawValue();

		Records.Add(Rec);
	}

	OK;
}
//---------------------------------------------------------------------

} //namespace Data