#pragma once
#include <Render/Technique.h>
#include <Data/StringID.h>
#include <Data/Dictionary.h>

// A complete (possibly multipass) shading effect on an abstract input data.
// Since input data may be different, although the desired shading effect is the same, an effect
// incapsulates a family of techniques, each of which implements an effect for a specific input data.
// An unique set of feature flags defines each input data case, so techniques are mapped to these
// flags, and clients can request an apropriate tech by its features. Request by ID is supported too.

namespace IO
{
	class CStream;
	class CBinaryReader;
}

namespace Render
{
typedef Ptr<class CEffect> PEffect;

class CEffect: public Data::CRefCounted
{
protected:

	EEffectType						Type;

	CDict<CStrID, PTechnique>		TechsByName;
	CDict<UPTR, PTechnique>			TechsByInputSet;

	//!!!need binary search by ID in fixed arrays!
	CDict<CStrID, void*>			DefaultConsts;
	CDict<CStrID, PTexture>			DefaultResources;
	CDict<CStrID, PSampler>			DefaultSamplers;
	char*							pMaterialConstDefaultValues = nullptr;
	UPTR							MaterialConstantBufferCount = 0;

	//CShaderParamTable GlobalParams; // Stored in RP, used only there
	CShaderParamTable MaterialParams;

public:

	CEffect();
	~CEffect();

	bool								IsValid() const { return !!TechsByInputSet.GetCount(); }

	const CTechnique* GetTechByName(CStrID Name) const;
	const CTechnique* GetTechByInputSet(UPTR InputSet) const;

	EEffectType       GetType() const { return Type; }
	const auto&       GetMaterialParamTable() const { return MaterialParams; }
	void*             GetConstantDefaultValue(CStrID ID) const;
	PTexture          GetResourceDefaultValue(CStrID ID) const;
	PSampler          GetSamplerDefaultValue(CStrID ID) const;
};

}
