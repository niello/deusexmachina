#include "NodeControllerPriorityBlend.h"

namespace Anim
{

bool CNodeControllerPriorityBlend::AddSource(Scene::CNodeController& Ctlr, DWORD Priority, float Weight)
{
#ifdef _DEBUG
	for (int i = 0; i < Sources.GetCount(); ++i)
		if (Sources[i].Ctlr.GetUnsafe() == &Ctlr) FAIL;
#endif

	if (Sources.GetCount())
	{
		if (IsLocalSpace() != Ctlr.IsLocalSpace()) FAIL;
		if (!NeedToUpdateLocalSpace() && Ctlr.NeedToUpdateLocalSpace())
			Flags.Set(UpdateLocalSpace);
	}
	else
	{
		Flags.SetTo(LocalSpace, Ctlr.IsLocalSpace());
		Flags.SetTo(UpdateLocalSpace, Ctlr.NeedToUpdateLocalSpace());
	}

	CSource Src;
	Src.Ctlr = &Ctlr;
	Src.Priority = Priority;
	Src.Weight = Weight;
	Sources.InsertSorted(Src);

	Channels.Set(Ctlr.GetChannels());
	OK;
}
//---------------------------------------------------------------------

void CNodeControllerPriorityBlend::RemoveSource(Scene::CNodeController& Ctlr)
{
	for (int i = 0; i < Sources.GetCount(); ++i)
		if (Sources[i].Ctlr.GetUnsafe() == &Ctlr)
		{
			Sources.EraseAt(i);
			Channels.ClearAll();
			for (int j = 0; j < Sources.GetCount(); ++j)
				Channels.Set(Sources[j].Ctlr->GetChannels());
			return;
		}
}
//---------------------------------------------------------------------

void CNodeControllerPriorityBlend::Clear()
{
	Channels.ClearAll();
	Sources.Clear();
}
//---------------------------------------------------------------------

bool CNodeControllerPriorityBlend::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!Sources.GetCount() || !Channels.IsAny(Scene::Chnl_Scaling | Scene::Chnl_Rotation | Scene::Chnl_Translation)) FAIL;

	bool HasActiveSrcs = false;
	for (int i = 0; i < Sources.GetCount(); ++i)
	{
		CSource& Src = Sources[i];
		if (Src.Ctlr->IsActive())
			HasActiveSrcs |= Src.Ctlr->ApplyTo(Src.SRT);
	}

	if (!HasActiveSrcs) FAIL;

	if (HasChannel(Scene::Chnl_Scaling))
	{
		DestTfm.Scale.set(0.f, 0.f, 0.f);
		float TotalWeight = 0.f;
		for (int i = 0; i < Sources.GetCount(); ++i)
		{
			CSource& Src = Sources[i];
			if (!Src.Ctlr->IsActive() || !Src.Ctlr->HasChannel(Scene::Chnl_Scaling) || Src.Weight == 0.f) continue;

			float WeightLeft = 1.f - TotalWeight;
			bool TheLast = (Src.Weight >= WeightLeft);
			float Weight = TheLast ? WeightLeft : Src.Weight;
			DestTfm.Scale += Src.SRT.Scale * Weight;
			TotalWeight += Weight;

			if (TheLast) break;
		}

		if (TotalWeight == 0.f) DestTfm.Scale.set(1.f, 1.f, 1.f);
	}

	if (HasChannel(Scene::Chnl_Rotation))
	{
		DestTfm.Rotation.set(0.f, 0.f, 0.f, 1.f);
		float TotalWeight = 0.f;
		for (int i = 0; i < Sources.GetCount(); ++i)
		{
			CSource& Src = Sources[i];
			if (!Src.Ctlr->IsActive() || !Src.Ctlr->HasChannel(Scene::Chnl_Rotation) || Src.Weight == 0.f) continue;

			float WeightLeft = 1.f - TotalWeight;
			bool TheLast = (Src.Weight >= WeightLeft);
			float Weight = TheLast ? WeightLeft : Src.Weight;

			//!!!Need quaternion interpolation that support more than one quaternion! nlerp, squad?
			float RelTotalWeight = TotalWeight;
			TotalWeight += Weight;
			RelTotalWeight /= TotalWeight;
			if (RelTotalWeight > 0.f)
			{
				quaternion Tmp = DestTfm.Rotation;
				DestTfm.Rotation.slerp(Tmp, Src.SRT.Rotation, RelTotalWeight);
			}
			else DestTfm.Rotation = Src.SRT.Rotation;

			if (TheLast) break;
		}
	}

	if (HasChannel(Scene::Chnl_Translation))
	{
		DestTfm.Translation.set(0.f, 0.f, 0.f);
		float TotalWeight = 0.f;
		for (int i = 0; i < Sources.GetCount(); ++i)
		{
			CSource& Src = Sources[i];
			if (!Src.Ctlr->IsActive() || !Src.Ctlr->HasChannel(Scene::Chnl_Translation) || Src.Weight == 0.f) continue;

			float WeightLeft = 1.f - TotalWeight;
			bool TheLast = (Src.Weight >= WeightLeft);
			float Weight = TheLast ? WeightLeft : Src.Weight;
			DestTfm.Translation += Src.SRT.Translation * Weight;
			TotalWeight += Weight;

			if (TheLast) break;
		}
	}

	OK;
}
//---------------------------------------------------------------------

}
