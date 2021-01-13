#include "ClipPlayerNode.h"
#include <Animation/AnimationController.h>
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>

//!!!DBG TMP!
#include <Animation/PoseOutput.h>

namespace DEM::Anim
{

//!!!DBG TMP!
//!!!FIXME: revisit CMappedPoseOutput and this!
class CStackMappedPoseOutput : public IPoseOutput
{
protected:

	IPoseOutput& _Output;
	U16*         _PortMapping;

public:

	CStackMappedPoseOutput(IPoseOutput& Output, U16* PortMapping) : _Output(Output), _PortMapping(PortMapping) {}

	virtual U8   GetActivePortChannels(U16 Port) const override { return _Output.GetActivePortChannels(_PortMapping[Port]); }

	virtual void SetScale(U16 Port, const vector3& Scale) override { return _Output.SetScale(_PortMapping[Port], Scale); }
	virtual void SetRotation(U16 Port, const quaternion& Rotation) override { return _Output.SetRotation(_PortMapping[Port], Rotation); }
	virtual void SetTranslation(U16 Port, const vector3& Translation) override { return _Output.SetTranslation(_PortMapping[Port], Translation); }
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override { return _Output.SetTransform(_PortMapping[Port], Tfm); }
};

CClipPlayerNode::CClipPlayerNode(CStrID ClipID, bool Loop, float Speed, float StartTime)
	: _ClipID(ClipID)
	, _StartTime(StartTime)
	, _Speed(Speed)
	, _Loop(Loop)
{
}
//---------------------------------------------------------------------

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

			if (PortMapping.empty())
			{
				_PortMapping.reset();
			}
			else
			{
				auto EmptyIt = std::find(PortMapping.cbegin(), PortMapping.cend(), CSkeletonInfo::EmptyPort);
				if (EmptyIt != PortMapping.cend())
				{
					// Create our own copy instead of modifying a shared one
					if (Context.SkeletonInfo->GetRefCount() > 1)
						Context.SkeletonInfo = n_new(CSkeletonInfo(*Context.SkeletonInfo));

					const auto StartIdx = static_cast<size_t>(std::distance(PortMapping.cbegin(), EmptyIt));
					Anim->GetSkeletonInfo().MergeInto(*Context.SkeletonInfo, PortMapping, StartIdx);
				}

				_PortMapping.reset(new U16[PortMapping.size()]);
				std::memcpy(_PortMapping.get(), PortMapping.data(), sizeof(U16) * PortMapping.size());
			}
		}

		_Sampler.SetClip(Anim);
	}
}
//---------------------------------------------------------------------

void CClipPlayerNode::Update(CAnimationController& Controller, float dt)
{
	if (_Sampler.GetClip())
		_CurrClipTime = _Sampler.GetClip()->AdjustTime(_CurrClipTime + dt * _Speed, _Loop);

	//???where to handle synchronization? pass sync info into a context and sync after graph update?
}
//---------------------------------------------------------------------

void CClipPlayerNode::EvaluatePose(IPoseOutput& Output)
{
	if (_PortMapping)
	{
		CStackMappedPoseOutput MappedOutput(Output, _PortMapping.get());
		_Sampler.Apply(_CurrClipTime, MappedOutput);
	}
	else
	{
		_Sampler.Apply(_CurrClipTime, Output);
	}
}
//---------------------------------------------------------------------

}
