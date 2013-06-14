#include "NodeControllerPriorityBlend.h"

namespace Anim
{

//!!!TMP HACK!
bool CNodeControllerPriorityBlend::OnAttachToNode(Scene::CSceneNode* pSceneNode)
{
	for (int i = 0; i < Sources.GetCount(); ++i)
		Sources[i].Ctlr->OnAttachToNode(pSceneNode);
	OK;
}
//---------------------------------------------------------------------

//!!!TMP HACK!
void CNodeControllerPriorityBlend::OnDetachFromNode()
{
	for (int i = 0; i < Sources.GetCount(); ++i)
		Sources[i].Ctlr->OnDetachFromNode();
}
//---------------------------------------------------------------------

bool CNodeControllerPriorityBlend::AddSource(Scene::CNodeController& Ctlr, DWORD Priority, float Weight)
{
#ifdef _DEBUG
	for (int i = 0; i < Sources.GetCount(); ++i)
		if (Sources[i].Ctlr.GetUnsafe() == &Ctlr) FAIL;
#endif

	if (Ctlr.IsComposite()) FAIL;

	if (Sources.GetCount())
	{
		//???or autoconvert based on parent node? now it is possible!
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

	//!!!TMP HACK!
	Src.Ctlr->OnAttachToNode(pNode);

	Channels.Set(Ctlr.GetChannels());
	OK;
}
//---------------------------------------------------------------------

bool CNodeControllerPriorityBlend::RemoveSource(const Scene::CNodeController& Ctlr)
{
	for (int i = 0; i < Sources.GetCount(); ++i)
		if (Sources[i].Ctlr.GetUnsafe() == &Ctlr)
		{
			Sources.EraseAt(i);
			Channels.ClearAll();
			for (int j = 0; j < Sources.GetCount(); ++j)
				Channels.Set(Sources[j].Ctlr->GetChannels());
			OK;
		}
	FAIL;
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
			if (TotalWeight > 0.f)
			{
				//???can write inplace slerp?
				quaternion Tmp = DestTfm.Rotation;
				DestTfm.Rotation.slerp(Tmp, Src.SRT.Rotation, TotalWeight / (TotalWeight + Weight));
			}
			else DestTfm.Rotation = Src.SRT.Rotation;

			if (TheLast) break;

			TotalWeight += Weight;
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

			if (TheLast) break;

			TotalWeight += Weight;
		}
	}

	OK;
}
//---------------------------------------------------------------------

}
