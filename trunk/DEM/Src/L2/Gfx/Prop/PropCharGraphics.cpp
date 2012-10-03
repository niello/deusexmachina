#include "PropCharGraphics.h"

#include <Gfx/Event/GfxAddAttachment.h>
#include <Gfx/Event/GfxRemoveAttachment.h>
#include <Gfx/Event/GfxSetAnimation.h>
//#include <Gfx/Event/gfxaddskin.h>
//#include <Gfx/Event/gfxremskin.h>
//#include <Gfx/Event/gfxsetcharacterset.h>
#include <Game/Entity.h>
#include <Gfx/GfxServer.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

extern const matrix44 Rotate180;

namespace Attr
{
	DeclareAttr(GUID);
	DeclareAttr(Graphics);
	DeclareAttr(Transform);

	DefineString(AnimSet);
	DefineString(CharacterSet);
};

BEGIN_ATTRS_REGISTRATION(PropCharGraphics)
	RegisterString(AnimSet, ReadOnly);
	RegisterString(CharacterSet, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropCharGraphics, Properties::CPropGraphics);
ImplementFactory(Properties::CPropCharGraphics);
RegisterProperty(CPropCharGraphics);

CPropCharGraphics::~CPropCharGraphics()
{
	n_assert(Attachments.Size() == 0);
}
//---------------------------------------------------------------------

void CPropCharGraphics::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CPropGraphics::GetAttributes(Attrs);
	Attrs.Append(Attr::AnimSet);
	Attrs.Append(Attr::CharacterSet);
}
//---------------------------------------------------------------------

void CPropCharGraphics::Activate()
{
	CPropGraphics::Activate();

	PROP_SUBSCRIBE_NEVENT(GfxSetAnimation, CPropCharGraphics, OnGfxSetAnimation);
	PROP_SUBSCRIBE_NEVENT(GfxAddAttachment, CPropCharGraphics, OnGfxAddAttachment);
	PROP_SUBSCRIBE_NEVENT(GfxRemoveAttachment, CPropCharGraphics, OnGfxRemoveAttachment);
	PROP_SUBSCRIBE_PEVENT(OnRender, CPropCharGraphics, OnRender);
	PROP_SUBSCRIBE_PEVENT(OnEntityRenamed, CPropCharGraphics, OnEntityRenamed);
}
//---------------------------------------------------------------------

void CPropCharGraphics::Deactivate()
{
	UNSUBSCRIBE_EVENT(GfxSetAnimation);
	UNSUBSCRIBE_EVENT(GfxAddAttachment);
	UNSUBSCRIBE_EVENT(GfxRemoveAttachment);
	UNSUBSCRIBE_EVENT(OnRender);
	UNSUBSCRIBE_EVENT(OnEntityRenamed);

	Graphics::CLevel* GraphicsLevel = GfxSrv->GetLevel();
	n_assert(GraphicsLevel);
	for (int i = 0; i < Attachments.Size(); i++)
		if (Attachments[i].NewCreated)
			GraphicsLevel->RemoveEntity(Attachments[i].GraphicsEntity);
	Attachments.Clear();

	CPropGraphics::Deactivate();
}
//---------------------------------------------------------------------

void CPropCharGraphics::SetupGraphicsEntities()
{
	n_assert(GetEntity()->HasAttr(Attr::Transform));

	Ptr<Graphics::CCharEntity> GfxEntity = Graphics::CCharEntity::Create();
	GfxEntity->SetUserData(GetEntity()->GetUniqueID());
	GfxEntity->SetResourceName(GetEntity()->Get<nString>(Attr::Graphics));
	GfxEntity->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));

	GfxEntity->AnimMapping = GetEntity()->Get<nString>(Attr::AnimSet);
	if (!GfxEntity->AnimMapping.IsValid())
		n_error("CPropCharGraphics::SetupGraphicsEntity(): entity '%s' has empty AnimSet attribute!",
			GetEntity()->GetUniqueID().CStr());

	Graphics::CLevel* GraphicsLevel = GfxSrv->GetLevel();
	n_assert(GraphicsLevel);
	GraphicsLevel->AttachEntity(GfxEntity);

	////!!! mangalore: TODO : setting to character3mode dirty, only if graphic path matches a character3
	//nArray<nString> tokens;
	//nString pathToCharSet = GetEntity()->Get<nString>(Attr::Graphics);
	//int numTokens = pathToCharSet.Tokenize("/",tokens);
	//if ((numTokens == 3) && (tokens[0] == "characters") && (tokens[2] == "skeleton"))
	//{
	//	pathToCharSet = pathToCharSet.ExtractDirName() + "skinlists/";
	//	GfxEntity->LoadCharacter3Set(
	//		nString("gfxlib:")+pathToCharSet+GetEntity()->Get<nString>(Attr::CharacterSet)+".xml");
	//}

	GraphicsEntities.Append(GfxEntity.get());
}
//---------------------------------------------------------------------

int CPropCharGraphics::FindAttachment(const nString& JointName)
{
	n_assert(JointName.IsValid());

	//???what about multiple attachments to one joint?

	// resolve joint name into joint index
	Graphics::CCharEntity* CCharEntity = GetGraphicsEntity();
	int JointIndex = CCharEntity->GetJointIndexByName(JointName);
	if (JointIndex != INVALID_INDEX)
		for (int i = 0; i < Attachments.Size(); i++)
			if (JointIndex == Attachments[i].JointIndex)
				return i;

	return INVALID_INDEX;
}
//---------------------------------------------------------------------

bool CPropCharGraphics::OnRender(const CEventBase& Event)
{
	//CPropGraphics::OnRender();
	UpdateAttachments();
	OK;
}
//---------------------------------------------------------------------

bool CPropCharGraphics::OnGfxSetAnimation(const CEventBase& Event)
{
	const Event::GfxSetAnimation& e = (const Event::GfxSetAnimation&)Event;

	Graphics::CCharEntity* GfxEntity = GetGraphicsEntity();
	if (e.BaseAnim.IsValid() && e.BaseAnim != GfxEntity->GetBaseAnimation())
		GfxEntity->SetBaseAnimation(e.BaseAnim, e.FadeInTime, e.BaseAnimTimeOffset);
	if (e.StopOverlayAnim)
		GfxEntity->StopOverlayAnimation(e.FadeInTime);
	if (e.OverlayAnim.IsValid())
		GfxEntity->SetOverlayAnimation(e.OverlayAnim, e.FadeInTime, e.OverlayAnimDurationOverride);

	OK;
}
//---------------------------------------------------------------------

//void CPropCharGraphics::AddAttachment(const nString& JointName,
//									   const nString& GfxResName,
//									   const matrix44& OffsetMatrix,
//									   Graphics::CEntity* GfxEntity)
bool CPropCharGraphics::OnGfxAddAttachment(const CEventBase& Event)
{
	const Event::GfxAddAttachment& e = (const Event::GfxAddAttachment&)Event;

	n_assert(e.JointName.IsValid() && (e.GfxResourceName.IsValid() || e.GfxEntity));

	// first check if previous Attachment is identical to the current, do nothing in this case
	int AttachIdx = FindAttachment(e.JointName);
	if (AttachIdx != INVALID_INDEX &&
		Attachments[AttachIdx].GraphicsEntity.isvalid() &&
		Attachments[AttachIdx].GraphicsEntity->GetResourceName() == e.GfxResourceName)
		FAIL;

	RemoveAttachment(e.JointName);

	Graphics::CCharEntity* CCharEntity = GetGraphicsEntity();
	n_assert(CCharEntity && CCharEntity->IsA(Graphics::CCharEntity::RTTI));
	int JointIndex = CCharEntity->GetJointIndexByName(e.JointName);
	if (JointIndex != INVALID_INDEX)
	{
		bool NewCreated;
		Graphics::CShapeEntity* GfxEntity;

		// if no GfxEntity is offered create a new one by resource name
		if (e.GfxEntity)
		{
			GfxEntity = e.GfxEntity;
			NewCreated = false;
		}
		else
		{
			GfxEntity = Graphics::CShapeEntity::Create();
			GfxEntity->SetResourceName(e.GfxResourceName);
			GfxEntity->SetVisible(false);
			NewCreated = true;

			Graphics::CLevel* GraphicsLevel = GfxSrv->GetLevel();
			n_assert(GraphicsLevel);
			GraphicsLevel->AttachEntity(GfxEntity);
		}

		Attachment NewAttachment;
		NewAttachment.JointIndex = JointIndex;
		NewAttachment.OffsetMatrix = e.OffsetMatrix;
		NewAttachment.GraphicsEntity = GfxEntity;
		NewAttachment.NewCreated = NewCreated;
		Attachments.Append(NewAttachment);

		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropCharGraphics::OnGfxRemoveAttachment(const CEventBase& Event)
{
	RemoveAttachment(((const Event::GfxRemoveAttachment&)Event).JointName);
	OK;
}
//---------------------------------------------------------------------

bool CPropCharGraphics::OnEntityRenamed(const CEventBase& Event)
{
	if (GraphicsEntities.Size() > 0 && GraphicsEntities[0].isvalid())
		GraphicsEntities[0]->SetUserData(GetEntity()->GetUniqueID());
	OK;
}
//---------------------------------------------------------------------

void CPropCharGraphics::UpdateTransform()
{
	//???cache Attr::Transform matrix ref-ptr?
	GraphicsEntities[0]->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));
}
//---------------------------------------------------------------------

void CPropCharGraphics::RemoveAttachment(const nString& JointName)
{
	n_assert(JointName.IsValid());
	Graphics::CLevel* GraphicsLevel = GfxSrv->GetLevel();
	n_assert(GraphicsLevel);
	int Index;
	while ((Index = FindAttachment(JointName)) != -1)
	{
		if (Attachments[Index].NewCreated)
			GraphicsLevel->RemoveEntity(Attachments[Index].GraphicsEntity);
		Attachments.Erase(Index);
	}
}
//---------------------------------------------------------------------

void CPropCharGraphics::UpdateAttachments()
{
	if (!Attachments.Size()) return;
	
	Graphics::CCharEntity* pCharEntity = GetGraphicsEntity();
	if (!pCharEntity) return;

	// update Nebula character skeleton
	pCharEntity->EvaluateSkeleton();

	// get my entity's transformation
	matrix44 WorldMatrix = Rotate180 * pCharEntity->GetTransform();

	for (int i = 0; i < Attachments.Size(); i++)
	{
		const Attachment& CurrAttachment = Attachments[i];

		// get Attachment joint matrix and rotate it by 180 degree
		matrix44 JointMatrix = CurrAttachment.OffsetMatrix;
		JointMatrix.mult_simple(pCharEntity->GetJointMatrix(CurrAttachment.JointIndex));
		JointMatrix.mult_simple(WorldMatrix);

		// ...and update Attachment graphics entity
		Graphics::CShapeEntity* GfxEntity = CurrAttachment.GraphicsEntity;
		GfxEntity->SetTransform(JointMatrix);
		if (!GfxEntity->GetVisible()) GfxEntity->SetVisible(true);
	}
}
//---------------------------------------------------------------------

/*
// Makes the given skin visible if our graphics entity has a Character3.
void CPropCharGraphics::AddSkin(const nString& SkinName)
{
	n_assert(SkinName.IsValid());
	if (GetGraphicsEntity()->HasCharacter3Set())
	{
		nCharacter3Set* Character3Set = (nCharacter3Set*)GetGraphicsEntity()->GetCharacterSet();
		n_assert(Character3Set);
		Character3Set->SetSkinVisible(SkinName, true);
	}
}
//---------------------------------------------------------------------

// Makes the given skin invisible if our graphics entity has a Character3.
void CPropCharGraphics::RemoveSkin(const nString& SkinName)
{
	n_assert(SkinName.IsValid());
	if (GetGraphicsEntity()->HasCharacter3Set())
	{
		nCharacter3Set* Character3Set = (nCharacter3Set*)GetGraphicsEntity()->GetCharacterSet();
		n_assert(Character3Set);
		Character3Set->SetSkinVisible(SkinName, false);
	}
}
//---------------------------------------------------------------------

// Sets the character set of the graphics entity.
void CPropCharGraphics::SetCharacterSet(const nString& CharacterSetName)
{
	n_assert(CharacterSetName.IsValid());

	Graphics::CCharEntity* CCharEntity = GetGraphicsEntity();
	n_assert(CCharEntity);

	if (!CCharEntity->HasCharacter3Set()) return;

	//!!! mangalore: TODO: remove loaded skins/stop running animations?

	// build filename of character set
	//!!! mangalore: TODO : setting to character3mode dirty, only if graphic path matches a character3
	nArray<nString> Tokens;
	nString PathToCharSet = GetEntity()->Get<nString>(Attr::Graphics);
	int NumTokens = PathToCharSet.Tokenize("/", Tokens);
	if ((NumTokens == 3) && (Tokens[0] == "characters") && (Tokens[2] == "skeleton"))
	{
		PathToCharSet = PathToCharSet.ExtractDirName() + "skinlists/";

		nString FileName;
		FileName.Format("gfxlib:%s%s.xml", PathToCharSet.Get(), CharacterSetName.Get());

		CCharEntity->LoadCharacter3Set(FileName);
	}
	else
	{
		// invalid path to character set, I'm not sure if this is allowed to happen!?
		n_error("invalid path to character set: %s", PathToCharSet.Get());
	}
}
//---------------------------------------------------------------------
*/

} // namespace Properties
