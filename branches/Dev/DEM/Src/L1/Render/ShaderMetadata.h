#pragma once
#ifndef __DEM_L1_RENDER_SHADER_VARS_METADATA_H__
#define __DEM_L1_RENDER_SHADER_VARS_METADATA_H__

#include <Data/RefCounted.h>
#include <Data/StringID.h>
#include <Data/Dictionary.h>

// Describes a structure of a shader technique, including constants, resources (buffers and textures),
// sampler states, vertex input signature. Allows to instantiate variable buffers which manipulate
// with actual data.

namespace Render
{
typedef DWORD HShaderVar; // Opaque to user, so its internal meaning can be overridden in subclasses
class CShaderVars;

//???textures/shader resources too, or separate handles?
struct CShaderVarMetadata
{
	HShaderVar Handle;
	// Buffer index, Offset in buffer
	// Type (float, int, bool, ?texture? etc)
	// Size (when applicable)
	//???bitmask of shader types where this var is used? VS, PS etc
	//each buffer also can store that mask if needed (DX11) - useful for binding CBs
	//simple detection of unused variables
};

class CShaderMetadata: public Data::CRefCounted
{
protected:

	CDict<CStrID, CShaderVarMetadata> VarMeta;
	//???map by handle? for DX9 need to know var type.
	//handle can be a pointer to CShaderVarMetadata for DX9, to get type, or type can be packed inside
	// Input signature + hash/ID for faster lookup (mb store list per input sig and don't combine
	// input sig with vertex decl sig, as dynamic CStrID requires hash table lookup anyway).
	// Buffer descriptions

	virtual HShaderVar			CreateHandle(const CShaderVarMetadata& Meta) const = 0;

public:

	virtual CShaderVars*		CreateVarsInstance() = 0; //???return smartptr?

	const CShaderVarMetadata*	GetVarMetadata(CStrID Name) const;
};

typedef Ptr<CShaderMetadata> PShaderMetadata;

inline const CShaderVarMetadata* CShaderMetadata::GetVarMetadata(CStrID Name) const
{
	int Idx = VarMeta.FindIndex(Name);
	return Idx == INVALID_INDEX ? NULL : &VarMeta.ValueAt(Idx);
}
//---------------------------------------------------------------------

}

#endif
