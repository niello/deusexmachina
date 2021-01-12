#include "ClipPlayerNode.h"
#include <Animation/AnimationController.h>
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>

namespace DEM::Anim
{

void CClipPlayerNode::Init(CAnimationControllerInitContext& Context)
{
	_CurrClipTime = _StartTime;

	CStrID ClipID = _ClipID;
	if (!Context.AssetOverrides.empty())
	{
		auto It = Context.AssetOverrides.find(ClipID);
		if (It != Context.AssetOverrides.cend())
			ClipID = It->second;
	}

	auto AnimRsrc = Context.ResourceManager.RegisterResource<CAnimationClip>(ClipID.CStr());
	if (auto Anim = AnimRsrc->ValidateObject<CAnimationClip>())
	{
		if (!Context.SkeletonInfo)
		{
			// Share a clip's skeleton without copying
			Context.SkeletonInfo = &Anim->GetSkeletonInfo();
		}
		else if (Context.SkeletonInfo != &Anim->GetSkeletonInfo())
		{
			std::vector<U16> PortMapping;
			Anim->GetSkeletonInfo().MapTo(*Context.SkeletonInfo, PortMapping);

			auto EmptyIt = std::find(PortMapping.cbegin(), PortMapping.cend(), CSkeletonInfo::EmptyPort);
			if (EmptyIt != PortMapping.cend())
			{
				// Create our own copy instead of modifying a shared one
				if (Context.SkeletonInfo->GetRefCount() > 1)
					Context.SkeletonInfo = n_new(CSkeletonInfo(*Context.SkeletonInfo));

				const auto StartIdx = static_cast<size_t>(std::distance(PortMapping.cbegin(), EmptyIt));
				Anim->GetSkeletonInfo().MergeInto(*Context.SkeletonInfo, PortMapping, StartIdx);
			}

			//!!!PortMapping must be used in EvaluatePose if not empty!
			//???use stack-based CMappedPoseOutput?
			//???!!!store PortMapping in a fixed array?! unique_ptr<[]>, nullptr = direct mapping
		}

		_Sampler.SetClip(Anim);
	}
}
//---------------------------------------------------------------------

void CClipPlayerNode::Update(float dt/*, params*/)
{
	if (_Sampler.GetClip())
		_CurrClipTime = _Sampler.GetClip()->AdjustTime(_CurrClipTime + dt * _Speed, _Loop);

	//???where to handle synchronization? pass sync info into a context and sync after graph update?
}
//---------------------------------------------------------------------

void CClipPlayerNode::EvaluatePose(IPoseOutput& Output)
{
	_Sampler.Apply(_CurrClipTime, Output);
}
//---------------------------------------------------------------------

}
