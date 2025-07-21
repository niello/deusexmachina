#include "Params.h"

namespace Data
{
DEFINE_TYPE(PParams, PParams())

void CParams::FromDataDict(const std::map<CStrID, Data::CData>& Dict)
{
	Params.clear();
	Params.reserve(Dict.size());
	for (const auto& [Key, Value] : Dict)
		Params.push_back(CParam(Key, Value));
}
//---------------------------------------------------------------------

void CParams::ToDataDict(std::map<CStrID, Data::CData>& Dict) const
{
	Dict.clear();
	for (UPTR i = 0; i < Params.size(); ++i)
		Dict.emplace(Params[i].GetName(), Params[i].GetRawValue());
}
//---------------------------------------------------------------------

void CParams::Merge(const CParams& Other, int Method)
{
	for (const CParam& Prm : Other)
	{
		// null value is considered a deleter, not a value, if corresponding flag is set
		const bool IsDeleter = ((Method & Merge_DeleteNulls) && Prm.GetRawValue().IsVoid());

		CParam* pMyPrm;

		if (TryGet(pMyPrm, Prm.GetName()))
		{
			if (Method & Merge_Replace)
			{
				if (IsDeleter)
					Remove(Prm.GetName());
				else if ((Method & Merge_Deep) && Prm.IsA<PParams>() && pMyPrm->IsA<PParams>())
					pMyPrm->GetValue<PParams>()->Merge(*Prm.GetValue<PParams>(), Method);
				else
					pMyPrm->SetValue(Prm.GetRawValue());
			}
		}
		else if (Method & Merge_AddNew)
		{
			// Don't add deleters, they aren't values
			if (!IsDeleter) Params.push_back(Prm);
		}
	}
}
//---------------------------------------------------------------------

void CParams::MergeDiff(CParams& OutChangedData, const CParams& Diff) const
{
	// Add new fields from diff
	for (UPTR i = 0; i < Diff.GetCount(); ++i)
	{
		const CParam& Prm = Diff[i];
		if (!Prm.GetRawValue().IsVoid() && !Has(Prm.GetName()))
			OutChangedData.Set(Prm);
	}

	for (UPTR i = 0; i < Params.size(); ++i)
	{
		const CParam& Prm = Params[i];
		CParam* pDiffPrm;
		if (Diff.TryGet(pDiffPrm, Prm.GetName()))
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
	// Cnanged data no longer contains my param, set nullptr as its new value in diff
	for (UPTR i = 0; i < GetCount(); ++i)
	{
		CStrID ID = Params[i].GetName();
		if (!ChangedData.Has(ID)) OutDiff.Set(ID, Data::CData());
	}

	// Write fields added or changed in a changed data
	for (UPTR i = 0; i < ChangedData.GetCount(); ++i)
	{
		CStrID Key = ChangedData.Get(i).GetName();
		const Data::CData& ChangedVal = ChangedData.Get(i).GetRawValue();
		Data::CParam* pInitialParam;
		bool IsNew = !Params.size() || !TryGet(pInitialParam, Key);
		if (IsNew || pInitialParam->GetRawValue() != ChangedVal)
		{
			// Recursively diff CParams //???can recurse to CDataArray?
			if (!IsNew && ChangedVal.IsA<PParams>() && pInitialParam->IsA<PParams>())
			{
				PParams SubDiff = n_new(CParams);
				pInitialParam->GetValue<PParams>()->GetDiff(*SubDiff, *ChangedVal.GetValue<PParams>());
				if (SubDiff->GetCount()) OutDiff.Set(Key, SubDiff);
			}
			else OutDiff.Set(Key, ChangedVal);
		}
	}
}
//---------------------------------------------------------------------

void CParams::GetDiff(CParams& OutDiff, const std::map<CStrID, Data::CData>& ChangedData) const
{
	// Cnanged data no longer contains my param, set nullptr as its new value in diff
	for (UPTR i = 0; i < GetCount(); ++i)
	{
		CStrID ID = Params[i].GetName();
		if (ChangedData.find(ID) == ChangedData.cend()) OutDiff.Set(ID, Data::CData());
	}

	// Write fields added or changed in a changed data
	for (const auto& [Key, ChangedVal] : ChangedData)
	{
		Data::CParam* pInitialParam;
		bool IsNew = Params.empty() || !TryGet(pInitialParam, Key);
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

}
