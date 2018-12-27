#include "ShaderLibraryLoaderSLB.h"

#include <Render/ShaderLibrary.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClass(Resources::CShaderLibraryLoaderSLB, 'LSLB', Resources::CResourceLoader);

const Core::CRTTI& CShaderLibraryLoaderSLB::GetResultType() const
{
	return Render::CShaderLibrary::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CShaderLibraryLoaderSLB::Load(IO::CStream& Stream)
{
	IO::CBinaryReader Reader(Stream);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'SLIB') return NULL;

	U32 FormatVersion;
	if (!Reader.Read(FormatVersion)) return NULL;

	U32 ShaderCount;
	if (!Reader.Read(ShaderCount)) return NULL;

	Render::PShaderLibrary ShaderLibrary = n_new(Render::CShaderLibrary);

	ShaderLibrary->TOC.SetSize(ShaderCount);
	for (U32 i = 0; i < ShaderCount; ++i)
	{
		Render::CShaderLibrary::CRecord& Rec = ShaderLibrary->TOC[i];
		if (!Reader.Read(Rec.ID)) return NULL;
		if (!Reader.Read(Rec.Offset)) return NULL;
		if (!Reader.Read(Rec.Size)) return NULL;
	}

	// Own initialization stream for lazy loading of shaders.
	// Ensure Stream is properly ref-counted (not created on stack).
	n_assert_dbg(Stream.GetRefCount() > 0);
	if ((Stream.GetRefCount() > 0))
		ShaderLibrary->Storage = &Stream;

	return ShaderLibrary.GetUnsafe();
}
//---------------------------------------------------------------------

}
