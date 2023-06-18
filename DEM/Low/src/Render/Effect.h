#pragma once
#include <Render/Technique.h>
#include <Data/StringID.h>
#include <map>

// A complete (possibly multipass) shading effect on an abstract input data.
// Since input data may be different, although the desired shading effect is the same, an effect
// incapsulates a family of techniques, each of which implements an effect for a specific input data.
// An unique set of feature flags defines each input data case, so techniques are mapped to these
// flags, and clients can request an apropriate tech by its features. Request by ID is supported too.

namespace Render
{
typedef Ptr<class CEffect> PEffect;

struct CShaderConstValue
{
	void* pData;
	U32   Size;
};

struct CShaderParamValues
{
	std::map<CStrID, CShaderConstValue> ConstValues;
	std::map<CStrID, Render::PTexture> ResourceValues;
	std::map<CStrID, Render::PSampler> SamplerValues;
	std::unique_ptr<char[]> ConstValueBuffer;
};

class CEffect: public Data::CRefCounted
{
protected:

	EEffectType                  _Type;

	std::map<CStrID, PTechnique> _TechsByName;
	std::map<CStrID, PTechnique> _TechsByInputSet;

	PShaderParamTable            _MaterialParams;
	CShaderParamValues           _MaterialDefaults;

public:

	CEffect(EEffectType Type, PShaderParamTable MaterialParams, CShaderParamValues&& MaterialDefaults, std::map<CStrID, PTechnique>&& Techs);
	virtual ~CEffect() override;

	bool                     IsValid() const { return !_TechsByInputSet.empty(); }

	const CTechnique*        GetTechByName(CStrID Name) const;
	const CTechnique*        GetTechByInputSet(CStrID InputSet) const;

	EEffectType              GetType() const { return _Type; }
	CShaderParamTable&       GetMaterialParamTable() const { return *_MaterialParams; }
	const CShaderConstValue* GetConstantDefaultValue(CStrID ID) const;
	CTexture*                GetResourceDefaultValue(CStrID ID) const;
	CSampler*                GetSamplerDefaultValue(CStrID ID) const;
};

}
