#include "NodeControllerComposite.h"

namespace Scene
{
__ImplementClassNoFactory(Scene::CNodeControllerComposite, CNodeController);

bool CNodeControllerComposite::OnAttachToNode(Scene::CSceneNode* pSceneNode)
{
	if (!CNodeController::OnAttachToNode(pSceneNode)) FAIL;
	pNode = pSceneNode;
	for (UPTR i = 0; i < Sources.GetCount(); ++i)
		Sources[i].Ctlr->OnAttachToNode(pSceneNode);
	OK;
}
//---------------------------------------------------------------------

void CNodeControllerComposite::OnDetachFromScene()
{
	for (UPTR i = 0; i < Sources.GetCount(); ++i)
		Sources[i].Ctlr->OnDetachFromScene();
	CNodeController::OnDetachFromScene();
}
//---------------------------------------------------------------------

bool CNodeControllerComposite::AddSource(Scene::CNodeController& Ctlr, UPTR Priority, float Weight)
{
#ifdef _DEBUG
	for (UPTR i = 0; i < Sources.GetCount(); ++i)
		n_assert(Sources[i].Ctlr.GetUnsafe() != &Ctlr);
#endif

	if (Sources.GetCount())
	{
		if (!NeedToUpdateLocalSpace() && Ctlr.NeedToUpdateLocalSpace())
			Flags.Set(UpdateLocalSpace);
	}
	else
	{
		Flags.SetTo(LocalSpace, Ctlr.IsLocalSpace());
		Flags.SetTo(UpdateLocalSpace, Ctlr.NeedToUpdateLocalSpace());
	}

	Channels.Set(Ctlr.GetChannels());

	CSource Src;
	Src.Ctlr = &Ctlr;
	Src.Priority = Priority;
	Src.Weight = Weight;
	Sources.InsertSorted(Src);

	if (pNode) Src.Ctlr->OnAttachToNode(pNode);

	OK;
}
//---------------------------------------------------------------------

bool CNodeControllerComposite::RemoveSource(const Scene::CNodeController& Ctlr)
{
	for (UPTR i = 0; i < Sources.GetCount(); ++i)
	{
		CSource& Src = Sources[i];
		if (Src.Ctlr.GetUnsafe() == &Ctlr)
		{
			if (pNode) Src.Ctlr->OnDetachFromScene();

			Sources.RemoveAt(i);

			Channels.ClearAll();
			for (UPTR j = 0; j < Sources.GetCount(); ++j)
				Channels.Set(Sources[j].Ctlr->GetChannels());

			OK;
		}
	}
	FAIL;
}
//---------------------------------------------------------------------

void CNodeControllerComposite::Clear()
{
	Channels.ClearAll();
	Sources.Clear();
}
//---------------------------------------------------------------------

}
