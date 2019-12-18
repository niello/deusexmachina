#include "AnimationLoaderANM.h"
#include <Animation/AnimationClip.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <acl/core/compressed_clip.h>

namespace Resources
{

const Core::CRTTI& CAnimationLoaderANM::GetResultType() const
{
	return DEM::Anim::CAnimationClip::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CAnimationLoaderANM::CreateResource(CStrID UID)
{
	if (!pResMgr) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	//U32 Magic;
	//if (!Reader.Read(Magic) || Magic != 'ANIM') return nullptr;

	U32 ClipDataSize;
	if (!Reader.Read(ClipDataSize) || !ClipDataSize) return nullptr;

	// Clip size is a part of clip data, so it must be read again as a part of the clip
	Stream->Seek(-4, IO::Seek_Current);

	void* pBuffer = n_malloc_aligned(ClipDataSize, alignof(acl::CompressedClip));

	if (!pBuffer || Stream->Read(pBuffer, ClipDataSize) != ClipDataSize)
	{
		SAFE_FREE_ALIGNED(pBuffer);
		return nullptr;
	}

	auto pClip = reinterpret_cast<acl::CompressedClip*>(pBuffer);
	if (!pClip->is_valid(true).empty())
	{
		SAFE_FREE_ALIGNED(pBuffer);
		return nullptr;
	}

	return n_new(DEM::Anim::CAnimationClip(pClip));
}
//---------------------------------------------------------------------

}
