#ifndef N_CHARACTER2SET_H
#define N_CHARACTER2SET_H

// (C) 2005 RadonLabs GmbH

#include <Core/RefCounted.h>
#include "util/nstring.h"
#include "kernel/nRefCounted.h"

class nCharacter2Set: public nRefCounted
{
protected:

    nArray<nString> clipNames;
    nArray<float> clipWeights;

public:

    bool dirty;
    float fadeInTime;

	nCharacter2Set(): dirty(false), fadeInTime(1.0f) {}

    void AddClip(const nString& clipName, float clipWeight);
    void RemoveClip(const nString& clipName);
	void SetClipWeightAt(int index, float clipWeight) { clipWeights[index] = clipWeight; dirty = true; }
    int GetClipIndexByName(const nString& clipName) const;
	const nString& GetClipNameAt(int index) const { return clipNames[index]; }
	float GetClipWeightAt(int index) const { return clipWeights[index]; }
	int GetNumClips() const { return clipNames.Size(); }
    void ClearClips();
};

inline void nCharacter2Set::AddClip(const nString& clipName, float clipWeight)
{
    clipNames.PushBack(clipName);
    clipWeights.PushBack(clipWeight);
    dirty = true;
}

inline void nCharacter2Set::RemoveClip(const nString& clipName)
{
    int index = GetClipIndexByName(clipName);
    n_assert(index != -1);
    clipNames.Erase(index);
    clipWeights.Erase(index);
    dirty = true;
}

inline int nCharacter2Set::GetClipIndexByName(const nString& name) const
{
    for (int i = 0; i < clipNames.Size(); i++)
        if (clipNames[i] == name)
            return i;
    return -1;
}

inline void nCharacter2Set::ClearClips()
{
    clipWeights.Clear();
    clipNames.Clear();
    dirty = true;
}

#endif
