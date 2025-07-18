#include "SkinAttribute.h"
#include <Frame/SkinProcessorAttribute.h>
#include <Render/SkinInfo.h>
#include <Scene/SceneNode.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CSkinAttribute, 'SKIN', Scene::CNodeAttribute);

CSkinAttribute::~CSkinAttribute() = default;
//---------------------------------------------------------------------

bool CSkinAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'SKIF':
			{
				_SkinInfoUID = DataReader.Read<CStrID>();
				break;
			}
			case 'RSPH':
			{
				DataReader.Read(_RootSearchPath);
				break;
			}
			case 'ACBN':
			{
				_Flags.SetTo(Skin_AutocreateBones, DataReader.Read<bool>());
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CSkinAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	Resources::PResource Rsrc = ResMgr.RegisterResource<Render::CSkinInfo>(_SkinInfoUID.CStr());
	if (!Rsrc) return false;
	_SkinInfo = Rsrc->ValidateObject<Render::CSkinInfo>();
	if (IsActive())
		InitSkinPalette();
	return !!_SkinInfo;
}
//---------------------------------------------------------------------

void CSkinAttribute::OnActivityChanged(bool Active)
{
	// Reference a palette only when active. Unreferenced palettes are not updated.
	if (Active)
		InitSkinPalette();
	else
		_SkinPalette = nullptr;
}
//---------------------------------------------------------------------

// TODO: ensure that the palette is updated on reparenting us to another scene node!
void CSkinAttribute::InitSkinPalette()
{
	if (_SkinPalette || !_pNode || !_SkinInfo) return;

	auto pRootParent = _pNode->FindNodeByPath(_RootSearchPath.c_str());
	if (!pRootParent) return;

	auto pSkinProcessor = pRootParent->FindFirstAttribute<CSkinProcessorAttribute>();
	if (!pSkinProcessor)
	{
		PSkinProcessorAttribute SkinProcessor(n_new(CSkinProcessorAttribute));
		if (pRootParent->AddAttribute(*SkinProcessor))
			pSkinProcessor = SkinProcessor.Get();
	}
	if (!pSkinProcessor) return;

	_SkinPalette = pSkinProcessor->GetSkinPalette(_SkinInfo, _Flags.Is(Skin_AutocreateBones));
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CSkinAttribute::Clone()
{
	PSkinAttribute ClonedAttr = n_new(CSkinAttribute);
	ClonedAttr->_RootSearchPath = _RootSearchPath;
	ClonedAttr->_SkinInfoUID = _SkinInfoUID;
	ClonedAttr->_Flags.SetTo(Skin_AutocreateBones, _Flags.Is(Skin_AutocreateBones));
	return ClonedAttr;
}
//---------------------------------------------------------------------

}
