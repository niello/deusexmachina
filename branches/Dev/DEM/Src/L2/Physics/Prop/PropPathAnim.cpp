#include "PropPathAnim.h"

#include <Game/GameServer.h>
#include <Game/Entity.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>
#include <Physics/Prop/PropTransformable.h>
#include <Physics/Event/SetTransform.h>
#include <anim2/nanimationserver.h>

namespace Attr
{
	DefineString(AnimPath);
	DefineBool(AnimRelative);
	DefineBool(AnimLoop);
	DefineBool(AnimPlaying);
};

BEGIN_ATTRS_REGISTRATION(PropPathAnim)
	RegisterString(AnimPath, ReadOnly);
	RegisterBool(AnimRelative, ReadOnly);
	RegisterBoolWithDefault(AnimLoop, ReadOnly, true);
	RegisterBool(AnimPlaying, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropPathAnim, Game::CProperty);
ImplementFactory(Properties::CPropPathAnim);
ImplementPropertyStorage(CPropPathAnim, 32);
RegisterProperty(CPropPathAnim);

CPropPathAnim::CPropPathAnim(): AnimTime(0.0f)
{
}
//---------------------------------------------------------------------

CPropPathAnim::~CPropPathAnim()
{
	n_assert(!refAnimation.isvalid());
}
//---------------------------------------------------------------------

void CPropPathAnim::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CProperty::GetAttributes(Attrs);
	Attrs.Append(Attr::AnimPath);
	Attrs.Append(Attr::AnimRelative);
	Attrs.Append(Attr::AnimLoop);
	Attrs.Append(Attr::AnimPlaying);
}
//---------------------------------------------------------------------

void CPropPathAnim::Activate()
{
	CProperty::Activate();

	// check if we actually should do some animation...
	const nString& AnimPath = GetEntity()->Get<nString>(Attr::AnimPath);
	if (AnimPath.IsValid())
	{
		// create a Nebula2 animation object
		nAnimation* Anim = nAnimationServer::Instance()->NewMemoryAnimation(AnimPath);
		n_assert(Anim);
		Anim->SetFilename(AnimPath);
		if (!Anim->Load())
		{
			n_error("CPropPathAnim: could not load anim '%s'", AnimPath.Get());
			return;
		}
		refAnimation = Anim;

		// save Initial position if relative animation is requested
		if (GetEntity()->Get<bool>(Attr::AnimRelative))
			InitialMatrix = GetEntity()->Get<matrix44>(Attr::Transform);

		// setup anim loop type
		refAnimation->GetGroupAt(0).SetLoopType(GetEntity()->Get<bool>(Attr::AnimLoop) ?
			nAnimation::Group::Repeat : nAnimation::Group::Clamp);

		// start playback
		AnimTime = 0.f;
		GetEntity()->Set<bool>(Attr::AnimPlaying, true);

		PROP_SUBSCRIBE_PEVENT(PathAnimPlay, CPropPathAnim, OnPathAnimPlay);
		PROP_SUBSCRIBE_PEVENT(PathAnimStop, CPropPathAnim, OnPathAnimStop);
		PROP_SUBSCRIBE_PEVENT(PathAnimRewind, CPropPathAnim, OnPathAnimRewind);

		PROP_SUBSCRIBE_PEVENT(OnMoveBefore, CPropPathAnim, OnMoveBefore);
	}
}
//---------------------------------------------------------------------

void CPropPathAnim::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnMoveBefore);

	UNSUBSCRIBE_EVENT(PathAnimPlay);
	UNSUBSCRIBE_EVENT(PathAnimStop);
	UNSUBSCRIBE_EVENT(PathAnimRewind);

	if (refAnimation.isvalid())
	{
		refAnimation->Release();
		refAnimation.invalidate();
	}

	CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropPathAnim::OnMoveBefore(const CEventBase& Event)
{
	if (refAnimation.isvalid()) UpdateAnimation();
	OK;
}
//---------------------------------------------------------------------

bool CPropPathAnim::OnPathAnimPlay(const CEventBase& Event)
{
	GetEntity()->Set<bool>(Attr::AnimPlaying, true);
	OK;
}
//---------------------------------------------------------------------

bool CPropPathAnim::OnPathAnimStop(const CEventBase& Event)
{
	GetEntity()->Set<bool>(Attr::AnimPlaying, false);
	OK;
}
//---------------------------------------------------------------------

bool CPropPathAnim::OnPathAnimRewind(const CEventBase& Event)
{
	AnimTime = 0.f;
	OK;
}
//---------------------------------------------------------------------

void CPropPathAnim::UpdateAnimation()
{
	if (GetEntity()->Get<bool>(Attr::AnimPlaying))
	{
		// get translation, rotation and scale
		vector4 Keys[3];
		refAnimation->SampleCurves((float)AnimTime, 0, 0, 3, &Keys[0]);

		// build matrix from translation and rotation
		vector3 Translation(Keys[0].x, Keys[0].y, Keys[0].z);
		quaternion Rotation(Keys[1].x, Keys[1].y, Keys[1].z, Keys[1].w);
		
		//???why no scaling? if really denied don't sample it, else use sampled value!

		// So non-transformable entity can't be animated now
		Event::SetTransform Event(Rotation);
		// Commented for test! No point to mul E mtx on smth.
		//Event->Transform.mult_simple(matrix44(Rotation)); //!!!need matrix-quaternion mul!
		Event.Transform.translate(Translation);
		if (GetEntity()->Get<bool>(Attr::AnimRelative)) Event.Transform *= InitialMatrix;

		GetEntity()->FireEvent(Event);

		AnimTime += GameSrv->GetFrameTime();
	}
}
//---------------------------------------------------------------------

} // namespace Properties
