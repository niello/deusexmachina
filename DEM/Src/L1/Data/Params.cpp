#include "Params.h"

namespace Data
{
__ImplementClass(Data::CParams, 'PRMS', Core::CRefCounted);

DEFINE_TYPE(PParams)

void CParams::FromDataDict(const CDataDict& Dict)
{
	Params.Clear();
	CParam* pParam = Params.Reserve(Dict.GetCount());
	for (int i = 0; i < Dict.GetCount(); ++i, ++pParam)
		pParam->Set(Dict.KeyAt(i), Dict.ValueAt(i));
}
//---------------------------------------------------------------------

void CParams::ToDataDict(CDataDict& Dict) const
{
	Dict.Clear();
	Dict.BeginAdd(Params.GetCount());
	for (int i = 0; i < Params.GetCount(); ++i)
		Dict.Add(Params[i].GetName(), Params[i].GetRawValue());
	Dict.EndAdd();
}
//---------------------------------------------------------------------

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

void CParams::MergeDiff(CParams& OutChangedData, const CParams& Diff) const
{
	// Add new fields from diff
	for (int i = 0; i < Diff.GetCount(); ++i)
	{
		const CParam& Prm = Diff[i];
		if (!Prm.GetRawValue().IsVoid() && !Has(Prm.GetName()))
			OutChangedData.Set(Prm);
	}

	for (int i = 0; i < Params.GetCount(); ++i)
	{
		const CParam& Prm = Params[i];
		CParam* pDiffPrm;
		if (Diff.Get(pDiffPrm, Prm.GetName()))
		{
			if (pDiffPrm->GetRawValue().IsVoid()) continue;

			//???allow CDataArray merging, per-element one by one? use KeepOrder flag!
			if (Prm.IsA<PParams>() && pDiffPrm->IsA<PParams>())
			{
				PParams SubMerge = n_new(CParams);
				Prm.GetValue<PParams>()->MergeDiff(*SubMerge, *pDiffPrm->GetValue<PParams>());
				OutChangedData.Set(Prm.GetName(), SubMerge);
			}
			else OutChangedData.Set(*pDiffPrm);
		}
		else OutChangedData.Set(Prm);
	}
}
//---------------------------------------------------------------------

void CParams::GetDiff(CParams& OutDiff, const CParams& ChangedData) const
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
			if (!IsNew && ChangedVal.IsA<PParams>() && pInitialParam->IsA<PParams>())
			{
				PParams SubDiff = n_new(CParams);
				pInitialParam->GetValue<PParams>()->GetDiff(*SubDiff, *ChangedVal.GetValue<PParams>());
				OutDiff.Set(Key, SubDiff);
			}
			else OutDiff.Set(Key, ChangedVal);
		}
	}
}
//---------------------------------------------------------------------

void CParams::GetDiff(CParams& OutDiff, const CDataDict& ChangedData) const
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
			if (!IsNew && ChangedVal.IsA<PParams>() && pInitialParam->IsA<PParams>())
			{
				PParams SubDiff = n_new(CParams);
				pInitialParam->GetValue<PParams>()->GetDiff(*SubDiff, *ChangedVal.GetValue<PParams>());
				OutDiff.Set(ChangedData.KeyAt(i), SubDiff);
			}
			else OutDiff.Set(ChangedData.KeyAt(i), ChangedVal);
		}
	}
}
//---------------------------------------------------------------------

}