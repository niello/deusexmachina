#include "Params.h"

namespace Data
{
ImplementRTTI(Data::CParams, Core::CRefCounted);
ImplementFactory(Data::CParams);

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

} //namespace AI