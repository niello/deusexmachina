//------------------------------------------------------------------------------
//  nskinanimator_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nskinanimator.h"
#include "scene/nskinshapenode.h"
#include "scene/nrendercontext.h"
#include "anim2/nanimation.h"
#include "resource/nresourceserver.h"
#include "variable/nvariableserver.h"
#include "character/ncharacter2set.h"
#include <Data/BinaryReader.h>

nNebulaClass(nSkinAnimator, "nscenenode");

//------------------------------------------------------------------------------
/**
*/
nSkinAnimator::nSkinAnimator() :
    characterVarIndex(0),
    characterSetIndex(-1),
    animEnabled(true)
{
    this->skinShapeNodeClass = nKernelServer::Instance()->FindClass("nskinshapenode");
    n_assert(this->skinShapeNodeClass);
}

bool nSkinAnimator::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'MINA': // ANIM
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			SetAnim(Value);
			OK;
		}
		case 'NIOJ': // JOIN
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;

			BeginJoints(Count);
			for (short i = 0; i < Count; ++i)
			{
				//!!!REDUNDANCY! { [ { } ] } problem.
				// This is { }'s element count, which is always 5 (Parent, T, R, S, Name)
				n_assert(DataReader.Read<short>() == 5);

				int ParentIdx;
				if (!DataReader.Read(ParentIdx)) FAIL;
				vector3 Scale;
				if (!DataReader.Read(Scale)) FAIL;
				quaternion Rotation;
				if (!DataReader.Read(Rotation)) FAIL;
				vector3 Translation;
				if (!DataReader.Read(Translation)) FAIL;
				char Name[256];
				if (!DataReader.ReadString(Name, sizeof(Name))) FAIL;
				SetJoint(i, ParentIdx, Translation, Rotation, Scale, Name);
			}
			EndJoints();

			//!!!TMP! Save skeleton in new format
			//Data::PParams Desc = character.GetSkeleton().CreateBoneHierarchyDesc(-1);
			//DataSrv->SaveHRD("home:Kila.hrd", Desc);

			OK;
		}
		case 'PILC': // CLIP
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;

			char Key[256];

			clips.SetFixedSize(Count);
			for (short i = 0; i < Count; ++i)
			{
				if (!DataReader.ReadString(Key, sizeof(Key))) FAIL;
				SetClip(i, DataReader.Read<int>(), Key);
			}

			OK;
		}
		case 'LNHC': // CHNL
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			SetChannel(Value);
			OK;
		}
		case 'PTPL': // LPTP
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			LoopType = nAnimLoopType::FromString(Value);
			OK;
		}
		default: return nSceneNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

// Sets the "animation channel" which drives this animation.
// This could be something like "time", but the actual names are totally
// up to the application. The actual channel value will be pulled from
// the render context provided in the Animate() method.
void nSkinAnimator::SetChannel(const char* name)
{
	n_assert(name);
	HChannel = nVariableServer::Instance()->GetVariableHandleByName(name);
	HChannelOffset = nVariableServer::Instance()->GetVariableHandleByName((nString(name) + "Offset").Get());
}
//---------------------------------------------------------------------

// Return the animation channel which drives this animation.
const char* nSkinAnimator::GetChannel()
{
	return HChannel == nVariable::InvalidHandle ? NULL : nVariableServer::Instance()->GetVariableName(HChannel);
}
//---------------------------------------------------------------------

void nSkinAnimator::UnloadAnim()
{
	if (refAnim.isvalid())
	{
		refAnim->Release();
		refAnim.invalidate();
	}
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Load the animation resource file.
*/
bool
nSkinAnimator::LoadAnim()
{
    if (!this->refAnim.isvalid() && animName.IsValid())
    {
		nAnimation* anim = (nAnimation*)nResourceServer::Instance()->NewResource("nmemoryanimation", animName, nResource::Animation);
        n_assert(anim);
        if (!anim->IsLoaded())
        {
            anim->SetFilename(this->animName);
            if (!anim->Load())
            {
                n_printf("nSkinAnimator: Error loading anim file '%s'\n", this->animName.Get());
                anim->Release();
                return false;
            }
        }
        this->refAnim = anim;
        this->character.SetAnimation(anim);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Load the resources needed by this object.
*/
bool
nSkinAnimator::LoadResources()
{
    if (nSceneNode::LoadResources())
    {
        this->LoadAnim();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Unload the resources.
*/
void
nSkinAnimator::UnloadResources()
{
    nSceneNode::UnloadResources();
    this->UnloadAnim();
}

//------------------------------------------------------------------------------
/**
*/
void
nSkinAnimator::RenderContextCreated(nRenderContext* renderContext)
{
    nSceneNode::RenderContextCreated(renderContext);

    // see if resources need to be reloaded
    if (!this->AreResourcesValid())
    {
        this->LoadResources();
    }

    // create one character 2 object per instance
    nCharacter2* curCharacter = n_new(nCharacter2(this->character));
    n_assert(0 != curCharacter);
    curCharacter->SetSkinAnimator(this);

    // add default clip
    if (!this->clips.Empty())
    {
        nAnimStateInfo newState;
        newState.SetStateStarted(0.0f);
        newState.BeginClips(1);
        newState.SetClip(0, this->GetClipAt(0), 1.0f);
        newState.EndClips();
        curCharacter->SetActiveState(newState);
    }

    // create one character set per instance
    nCharacter2Set* characterSet = n_new(nCharacter2Set);
    n_assert(characterSet);

    // put frame persistent data in render context
    nVariable::Handle characterSetHandle = nVariableServer::Instance()->GetVariableHandleByName("charSetPointer");
    this->characterSetIndex = renderContext->AddLocalVar(nVariable(characterSetHandle, characterSet));
    nVariable::Handle characterHandle = nVariableServer::Instance()->GetVariableHandleByName("charPointer");
    this->characterVarIndex = renderContext->AddLocalVar(nVariable(characterHandle, curCharacter));
}

//------------------------------------------------------------------------------
/**
    - 15-Jan-04     floh    AreResourcesValid()/LoadResource() moved to scene server
*/
void
nSkinAnimator::Animate(nSceneNode* sceneNode, nRenderContext* renderContext)
{
    n_assert(sceneNode);
    n_assert(renderContext);
    n_assert(nVariable::InvalidHandle != this->HChannel);

    const nVariable& characterVar = renderContext->GetLocalVar(this->characterVarIndex);
    nCharacter2* curCharacter = (nCharacter2*)characterVar.GetObj();
    n_assert(curCharacter);

    // update the animation enabled flag
    curCharacter->SetAnimEnabled(this->animEnabled);

    // check if I am already uptodate for this frame
    uint curFrameId = renderContext->GetFrameId();
    if (curCharacter->GetLastEvaluationFrameId() != curFrameId)
    {
        curCharacter->SetLastEvaluationFrameId(curFrameId);

        // get the sample time from the render context
        nVariable* var = renderContext->GetVariable(this->HChannel);
        n_assert2(var, "nSkinAnimator::Animate: TimeChannel Variable in RenderContext.\n");
        float curTime = var->GetFloat();

        // get the time offset from the render context
        var = renderContext->GetVariable(this->HChannelOffset);
        float curOffset = var ? var->GetFloat() : 0.0f;

        const nVariable& character2SetVar = renderContext->GetLocalVar(this->characterSetIndex);
        nCharacter2Set* characterSet = (nCharacter2Set*)character2SetVar.GetObj();
        n_assert(characterSet);

        // get character 2 set from render context and check if animation state needs to be updated
        if (characterSet->IsDirty())
        {
            nAnimStateInfo newState;
            int numClips = characterSet->GetNumClips();

            float weightSum = 0.0f;
            for (int i = 0; i < numClips; i++)
                weightSum += characterSet->GetClipWeightAt(i);

            // add clips
            if (weightSum > 0)
            {
                newState.SetStateStarted(curTime);
                newState.SetFadeInTime(characterSet->GetFadeInTime());
                newState.BeginClips(numClips);
                for (int i = 0; i < numClips; i++)
                {
                    int index = this->GetClipIndexByName(characterSet->GetClipNameAt(i));
                    if (-1 == index)
                        n_error("nSkinAnimator::Animate(): Requested clip \"%s\" does not exist.\n", characterSet->GetClipNameAt(i).Get());
                    newState.SetClip(i, this->GetClipAt(index), characterSet->GetClipWeightAt(i) / weightSum);
                }
                newState.EndClips();
            }

            curCharacter->SetActiveState(newState);
            characterSet->SetDirty(false);
        }

        // evaluate the current state of the character skeleton
        curCharacter->EvaluateSkeleton(curTime);
    }

    // update the source node with the new char skeleton state
    if (sceneNode->IsA(this->skinShapeNodeClass))
        ((nSkinShapeNode*)sceneNode)->SetCharSkeleton(&curCharacter->GetSkeleton());
    else n_error("nSkinAnimator::Animate(): invalid scene node class\n");
}

//------------------------------------------------------------------------------
/**
*/
void
nSkinAnimator::RenderContextDestroyed(nRenderContext* renderContext)
{
    // delete character object
    nVariable var = renderContext->GetLocalVar(this->characterVarIndex);
    nCharacter2* curCharacter = (nCharacter2*)var.GetObj();
	n_assert(curCharacter);// && curCharacter->GetRefCount() == 1);
    curCharacter->Release();

    // delete character set
    n_assert(0 != renderContext);
    var = renderContext->GetLocalVar(this->characterSetIndex);
    nCharacter2Set* characterSet = (nCharacter2Set*)var.GetObj();
    n_assert(characterSet);// && characterSet->GetRefCount() == 1);
    characterSet->Release();
}

//------------------------------------------------------------------------------
/**
*/
void
nSkinAnimator::SetClip(int clipIndex, int animGroupIndex, const nString& clipName)
{
    // number of anim curves in a clip is identical to number of (joints * 3)
    // (one curve for translation, rotation and scale)
    const int numCurves = this->GetNumJoints() * 3;
    n_assert(numCurves > 0);

    nAnimClip newClip(clipName, animGroupIndex, numCurves);
    this->clips.At(clipIndex) = newClip;
}

//------------------------------------------------------------------------------
/**
*/
int
nSkinAnimator::GetNumClips() const
{
    return this->clips.Size();
}

//------------------------------------------------------------------------------
/**
*/
const nAnimClip&
nSkinAnimator::GetClipAt(int clipIndex) const
{
    return this->clips[clipIndex];
}

//------------------------------------------------------------------------------
/**
*/
nTime
nSkinAnimator::GetClipDuration(int clipIndex) const
{
    const nAnimClip& clip = this->GetClipAt(clipIndex);
    return this->refAnim->GetDuration(clip.GetAnimGroupIndex());
}

//------------------------------------------------------------------------------
/**
*/
int
nSkinAnimator::GetClipIndexByName(const nString& name) const
{
    for (int i = 0; i < this->clips.Size(); i++)
    {
        if (this->clips[i].GetClipName() == name)
        {
            return i;
        }
    }

    return -1;
}

//------------------------------------------------------------------------------
/**
    Begin adding animation event tracks to a clip.
*/
void
nSkinAnimator::BeginAnimEventTracks(int clipIndex, int numTracks)
{
    this->clips.At(clipIndex).SetNumAnimEventTracks(numTracks);
}

//------------------------------------------------------------------------------
/**
    Begin adding events to an animation event track.
*/
void
nSkinAnimator::BeginAnimEventTrack(int clipIndex, int trackIndex, const nString& name, int numEvents)
{
    nAnimClip& clip = this->clips.At(clipIndex);
    nFixedArray<nAnimEventTrack>& animEventTracks = clip.GetAnimEventTracks();
    nAnimEventTrack& t = animEventTracks[trackIndex];
    t.SetName(name);
    t.SetNumEvents(numEvents);
}

//------------------------------------------------------------------------------
/**
    Set an animation event in a track
*/
void
nSkinAnimator::SetAnimEvent(int clipIndex, int trackIndex, int eventIndex, float time, const vector3& translate, const quaternion& rotate, const vector3& scale)
{
    nAnimClip& clip = this->clips.At(clipIndex);
    nFixedArray<nAnimEventTrack>& animEventTracks = clip.GetAnimEventTracks();
    nAnimEventTrack& t = animEventTracks[trackIndex];
    nAnimEvent e;
    e.SetTime(time);
    e.SetTranslation(translate);
    e.SetQuaternion(rotate);
    e.SetScale(scale);
    t.SetEvent(eventIndex, e);
}

//------------------------------------------------------------------------------
/**
*/
int 
nSkinAnimator::GetJointByName(const char* jointName)
{
    n_assert(jointName);
    return this->character.GetSkeleton().GetJointIndexByName(jointName);
}
