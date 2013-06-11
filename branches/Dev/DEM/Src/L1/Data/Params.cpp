#include "Params.h"

namespace Data
{
__ImplementClass(Data::CParams, 'PRMS', Core::CRefCounted);

DEFINE_TYPE(PParams)

void CParams::Merge(const CParams& Other, int Method)
{
	for (int i = 0; i < Other.GetCount(); ++i)
	{
		const CParam& Prm = Other.Get(i);
		CParam* pMyPrm;
		
		if (Get(pMyPrm, Prm.GetName()))
		{
			if (Method & Merge_Replace)
			{
				if ((Method & Merge_Deep) && Prm.IsA<PParams>() && pMyPrm->IsA<PParams>())
					pMyPrm->GetValue<PParams>()->Merge(*Prm.GetValue<PParams>(), Method);
				else pMyPrm->SetValue(Prm.GetRawValue());
			}
		}
		else if (Method & Merge_AddNew) Params.Append(Prm);
	}
}
//---------------------------------------------------------------------

void CParams::Diff(CParams& OutDiff, const CParams& ChangedData) const
{
	// Cnanged data no longer contains my param, set NULL as its new value in diff
	for (int i = 0; i < GetCount(); ++i)
	{
		CStrID ID = Params[i].GetName();
		if (!ChangedData.Has(ID)) OutDiff.Set(ID, Data::CData());
	}

	// Write fields added or changed in a changed data
	for (int i = 0; i < ChangedData.GetCount(); ++i)
	{
		CStrID Key = ChangedData.Get(i).GetName();
		const Data::CData& ChangedVal = ChangedData.Get(i).GetRawValue();
		Data::CParam* pInitialParam;
		bool IsNew = !Params.GetCount() || !Get(pInitialParam, Key);
		if (IsNew || pInitialParam->GetRawValue() != ChangedVal)
		{
			// Recursively diff CParams //???can recurse to CDataArray?
			if (!IsNew && ChangedVal.IsA<PParams>())
			{
				PParams SubDiff = n_new(CParams);
				pInitialParam->GetValue<PParams>()->Diff(*SubDiff, *ChangedVal.GetValue<PParams>());
				OutDiff.Set(Key, SubDiff);
			}
			else OutDiff.Set(Key, ChangedVal);
		}
	}
}
//---------------------------------------------------------------------

void CParams::Diff(CParams& OutDiff, const CDataDict& ChangedData) const
{
	// Cnanged data no longer contains my param, set NULL as its new value in diff
	for (int i = 0; i < GetCount(); ++i)
	{
		CStrID ID = Params[i].GetName();
		if (!ChangedData.Contains(ID)) OutDiff.Set(ID, Data::CData());
	}

	// Write fields added or changed in a changed data
	for (int i = 0; i < ChangedData.GetCount(); ++i)
	{
		Data::CParam* pInitialParam;
		bool IsNew = !Params.GetCount() || !Get(pInitialParam, ChangedData.KeyAt(i));
		const Data::CData& ChangedVal = ChangedData.ValueAt(i);
		if (IsNew || pInitialParam->GetRawValue() != ChangedVal)
		{
			// Recursively diff CParams //???can recurse to CDataArray?
			if (!IsNew && ChangedVal.IsA<PParams>())
			{
				PParams SubDiff = n_new(CParams);
				pInitialParam->GetValue<PParams>()->Diff(*SubDiff, *ChangedVal.GetValue<PParams>());
				OutDiff.Set(ChangedData.KeyAt(i), SubDiff);
			}
			else OutDiff.Set(ChangedData.KeyAt(i), ChangedVal);
		}
	}
}
//---------------------------------------------------------------------

}