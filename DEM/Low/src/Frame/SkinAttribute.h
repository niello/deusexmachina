#pragma once
#include <Scene/NodeAttribute.h>
#include <Math/Matrix44.h>

// Skin attribute adds a skinned rendering capability to sibling models, providing an access to the skin palette for them

namespace Render
{
	typedef Ptr<class CSkinInfo> PSkinInfo;
};

namespace Frame
{
using PSkinPalette = Ptr<class CSkinPalette>;

class CSkinAttribute : public Scene::CNodeAttribute
{
	FACTORY_CLASS_DECL;

protected:

	enum // extends Scene::CNodeAttribute enum
	{
		// Active
		// WorldMatrixChanged
		Skin_AutocreateBones = 0x10
	};

	std::string  _RootSearchPath;
	CStrID       _SkinInfoUID;
	PSkinPalette _SkinPalette;

public:

	virtual ~CSkinAttribute() override;

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual bool                  ValidateResources(Resources::CResourceManager& ResMgr) override;
	virtual Scene::PNodeAttribute Clone() override;

	const CSkinPalette*           GetSkinPalette() const { return _SkinPalette.Get(); }
};

typedef Ptr<CSkinAttribute> PSkinAttribute;

}
