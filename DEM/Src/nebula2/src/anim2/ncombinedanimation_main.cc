//------------------------------------------------------------------------------
//  nmemoryanimation_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "anim2/ncombinedanimation.h"
#include "mathlib/quaternion.h"
#include <kernel/nkernelserver.h>

nNebulaClass(nCombinedAnimation, "nmemoryanimation");

//------------------------------------------------------------------------------
/**
*/
nCombinedAnimation::nCombinedAnimation()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nCombinedAnimation::~nCombinedAnimation()
{
}

//------------------------------------------------------------------------------
/**
*/
bool
nCombinedAnimation::LoadResource()
{
    bool success = false;
    success = nAnimation::LoadResource();
    if (success)
    {
        this->SetState(Valid);
    };
    return success;
}

//------------------------------------------------------------------------------
/**
*/
void
nCombinedAnimation::UnloadResource()
{
    if (!this->IsUnloaded())
    {
        nAnimation::UnloadResource();
        this->SetState(Unloaded);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
nCombinedAnimation::BeginAnims()
{
    this->animPtrs.Clear();
};

//------------------------------------------------------------------------------
/**
*/
void
nCombinedAnimation::AddAnim(nMemoryAnimation* animation)
{
    this->animPtrs.Append(animation);
};

//------------------------------------------------------------------------------
/**
*/
void
nCombinedAnimation::EndAnims()
{
    // calculate space needed
    int i,k;
    int numGroups = 0;
    int numKeys = 0;
    for (i = 0; i < this->animPtrs.Size(); i++)
    {
        numGroups += this->animPtrs[i]->GetNumGroups();
        numKeys += this->animPtrs[i]->GetKeyArray().Size();
    };

    this->SetNumGroups(numGroups);
    this->keyArray.SetFixedSize(numKeys);

    // copy all data
    int currentGroup = 0;
    int currentKey = 0;
    int keyAnimOffset;
    for (i = 0; i < this->animPtrs.Size(); i++)
    {
        keyAnimOffset = currentKey;

        for (k = 0; k < this->animPtrs[i]->GetNumGroups(); k++)
        {
            nAnimation::Group &srcGroup = this->animPtrs[i]->GetGroupAt(k);
            nAnimation::Group &dstGroup = this->GetGroupAt(currentGroup);

            dstGroup.SetNumCurves(srcGroup.GetNumCurves());
            dstGroup.SetStartKey(srcGroup.GetStartKey());
            dstGroup.SetNumKeys(srcGroup.GetNumKeys());
            dstGroup.SetKeyStride(srcGroup.GetKeyStride());
            dstGroup.SetKeyTime(srcGroup.GetKeyTime());
            dstGroup.SetLoopType(srcGroup.GetLoopType());

            // copy curves
            int c;
            for (c = 0; c < srcGroup.GetNumCurves(); c++)
            {
                nAnimation::Curve &srcCurve = srcGroup.GetCurveAt(c);
                nAnimation::Curve &dstCurve = dstGroup.GetCurveAt(c);
                dstCurve.SetIpolType(srcCurve.GetIpolType());
                int srcFirstKey = srcCurve.GetFirstKeyIndex();
                if (srcFirstKey != -1)
                {
                    // add offset
                    srcFirstKey += keyAnimOffset;
                };
                dstCurve.SetFirstKeyIndex(srcFirstKey);
                dstCurve.SetConstValue(srcCurve.GetConstValue());
                dstCurve.SetIsAnimated(srcCurve.IsAnimated());
            };

            currentGroup++;
        };
        // copy keys
        for (k = 0; k < this->animPtrs[i]->GetKeyArray().Size(); k++)
        {
            this->keyArray[currentKey] = this->animPtrs[i]->GetKeyArray().At(k);
            currentKey++;
        };
    };

    // lets print some stats
    int numg = this->GetNumGroups();
    n_printf("Combined Animation Stats : %i Groups\n",numg);
    for (i = 0; i < numg; i++)
    {
        nAnimation::Group &group = this->GetGroupAt(i);
        n_printf("Group %i : Duration = %i, NumKeys = %i, StartKey = %i",i,group.GetDuration(),group.GetNumKeys(),group.GetStartKey());
        n_printf("\n");
    };
};

