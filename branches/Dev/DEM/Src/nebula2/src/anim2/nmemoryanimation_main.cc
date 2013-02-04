//------------------------------------------------------------------------------
//  nmemoryanimation_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "anim2/nmemoryanimation.h"
#include <Data/Streams/FileStream.h>
#include "mathlib/quaternion.h"
#include <kernel/nkernelserver.h>

nNebulaClass(nMemoryAnimation, "nanimation");

bool nMemoryAnimation::LoadResource()
{
    n_assert(IsUnloaded());

    bool success = false;
    nString filename = GetFilename();
    if (filename.CheckExtension("nax2")) success = LoadNax2(filename);
    else if (filename.CheckExtension("nanim2")) success = LoadNanim2(filename);
    if (success) SetState(Valid);
    return success;
}

//------------------------------------------------------------------------------
/**
*/
void
nMemoryAnimation::UnloadResource()
{
    if (!IsUnloaded())
    {
        nAnimation::UnloadResource();
        keyArray.Clear();
        SetState(Unloaded);
    }
}

//------------------------------------------------------------------------------
/**
*/
static inline bool GetS(Data::CFileStream* file, char* buf, int bufSize)
{
    n_assert(buf);
    n_assert(bufSize > 1);

    if (file->IsEOF()) return false;

    bufSize--; // make room for final terminating 0    

	int startPos = file->GetPosition();

    // read file contents in chunks of 64 bytes, not char by char
    int chunkSize = n_min(bufSize, 256);
    char* readPos = buf;
    int curIndex;
    for (curIndex = 0; curIndex < bufSize; curIndex++)
    {
        // read next chunk of data?
        if (0 == (curIndex % chunkSize))
        {
            // if we reached end-of-file before, break out
            if (file->IsEOF()) break;

            // now, read the next chunk of data
            int readSize = chunkSize;
            if ((curIndex + readSize) >= bufSize)
				readSize = bufSize - curIndex;
            int bytesRead = file->Read(readPos, readSize);
            if (bytesRead != readSize) readPos[bytesRead] = 0;
            readPos += bytesRead;
        }

        // check for newline
        if (buf[curIndex] == '\n')
        {
            // reset file pointer to position after new-line
			file->Seek(startPos + curIndex + 1, Data::SSO_BEGIN);
            break;
        }
    }
    buf[curIndex] = 0;
    return true;
}



//------------------------------------------------------------------------------
/**
    Loads animation data from an ASCII nanim2 file.
*/
bool
nMemoryAnimation::LoadNanim2(const nString& filename)
{
    n_assert(IsUnloaded())

	Data::CFileStream File;

    // open the file
	if (!File.Open(filename, Data::SAM_READ))
    {
        n_error("nMemoryAnimation::LoadNanim2(): Could not open file %s\n", filename);
        return false;
    }

    // read file line by line
    char line[1024];
    int groupIndex = 0;
    int curveIndex = 0;
    int keyIndex   = 0;
    Group* curGroup = 0;
    Curve* curCurve = 0;
    static vector4 vec4;
    while (GetS(&File, line, sizeof(line)))
    {
        // get keyword
        char* keyWord = strtok(line, N_WHITESPACE);
        if (0 == keyWord)
        {
            continue;
        }
        else if (0 == strcmp(keyWord, "type"))
        {
            // type must be 'nanim2'
            const char* typeString = strtok(0, N_WHITESPACE);
            n_assert(typeString);
            if (0 != strcmp(typeString, "nanim2"))
            {
                n_error("nMemoryAnimation::LoadNanim2(): File %s has invalid type %s, must be 'nanim2'\n", filename, typeString);
                File.Close();
                return false;
            }
        }
        else if (0 == strcmp(keyWord, "numgroups"))
        {
            const char* numGroupsString = strtok(0, N_WHITESPACE);
            n_assert(numGroupsString);

            int numGroups = atoi(numGroupsString);
            if (0 == numGroups)
            {
                n_error("nMemoryAnimation::LoadNanim2(): File %s has no groups! Invalid Export ?", filename);
                File.Close();
                return false;
            }

            SetNumGroups(numGroups);
        }
        else if (0 == strcmp(keyWord, "numkeys"))
        {
            const char* numKeysString = strtok(0, N_WHITESPACE);
            n_assert(numKeysString);
            keyArray.SetFixedSize(atoi(numKeysString));
        }
        else if (0 == strcmp(keyWord, "group"))
        {
            const char* numCurvesString = strtok(0, N_WHITESPACE);
            const char* startKeyString  = strtok(0, N_WHITESPACE);
            const char* numKeysString   = strtok(0, N_WHITESPACE);
            const char* keyStrideString = strtok(0, N_WHITESPACE);
            const char* keyTimeString   = strtok(0, N_WHITESPACE);
            const char* fadeInFramesString = strtok(0, N_WHITESPACE);
            const char* loopTypeString  = strtok(0, N_WHITESPACE);
            n_assert(numCurvesString && startKeyString && keyStrideString && numKeysString && keyTimeString && fadeInFramesString && loopTypeString);

            curveIndex = 0;
            curGroup = &(GetGroupAt(groupIndex++));
            curGroup->SetNumCurves(atoi(numCurvesString));
            curGroup->SetStartKey(atoi(startKeyString));
            curGroup->SetNumKeys(atoi(numKeysString));
            curGroup->SetKeyStride(atoi(keyStrideString));
            curGroup->SetKeyTime(float(atof(keyTimeString)));
            curGroup->SetFadeInFrames(float(atof(fadeInFramesString)));
            curGroup->SetLoopType(nAnimation::Group::StringToLoopType(loopTypeString));
        }
        else if (0 == strcmp(keyWord, "curve"))
        {
            const char* ipolTypeString      = strtok(0, N_WHITESPACE);
            const char* firstKeyIndexString = strtok(0, N_WHITESPACE);
            const char* isAnimatedString    = strtok(0, N_WHITESPACE);
            const char* constXString        = strtok(0, N_WHITESPACE);
            const char* constYString        = strtok(0, N_WHITESPACE);
            const char* constZString        = strtok(0, N_WHITESPACE);
            const char* constWString        = strtok(0, N_WHITESPACE);
            n_assert(ipolTypeString && firstKeyIndexString && isAnimatedString && constXString && constYString && constZString && constWString);

            n_assert(curGroup);
            curCurve = &(curGroup->GetCurveAt(curveIndex++));
            curCurve->SetIpolType(nAnimation::Curve::StringToIpolType(ipolTypeString));
            curCurve->SetFirstKeyIndex(atoi(firstKeyIndexString));
            curCurve->SetIsAnimated(atoi(isAnimatedString));
            vec4.x = float(atof(constXString));
            vec4.y = float(atof(constYString));
            vec4.z = float(atof(constZString));
            vec4.w = float(atof(constWString));
            curCurve->SetConstValue(vec4);
        }
        else if (0 == strcmp(keyWord, "key"))
        {
            const char* keyXString = strtok(0, N_WHITESPACE);
            const char* keyYString = strtok(0, N_WHITESPACE);
            const char* keyZString = strtok(0, N_WHITESPACE);
            const char* keyWString = strtok(0, N_WHITESPACE);
            n_assert(keyXString && keyYString && keyZString && keyWString);

            vec4.x = float(atof(keyXString));
            vec4.y = float(atof(keyYString));
            vec4.z = float(atof(keyZString));
            vec4.w = float(atof(keyWString));
            keyArray[keyIndex++] = vec4;
        }
        else
        {
            n_error("nMemoryAnimation::LoadNanim2(): Unknown keyword %s in nanim2 file %s\n", keyWord, filename);
            File.Close();
            return false;
        }
    }

    // cleanup
    File.Close();
    return true;
}

//------------------------------------------------------------------------------
/**
    Loads animation data from a binary nax2 file.

    - 30-Jun-04 floh    fixed assertion bug when number of keys in a curve is 0
*/
bool
nMemoryAnimation::LoadNax2(const nString& filename)
{
    n_assert(IsUnloaded());

	Data::CFileStream File;

    // open the file
	if (!File.Open(filename, Data::SAM_READ))
    {
        n_error("nMemoryAnimation::LoadNax2(): Could not open file %s!", filename);
        return false;
    }

    // read header
    int magic;
	File.Read(&magic, sizeof(int));
    if (magic != 'NAX2')
    {
        n_error("nMemoryAnimation::LoadNax2(): File %s is not a NAX2 file!", filename);
        File.Close();
        return false;
    }
    int numGroups;
	File.Read(&numGroups, sizeof(int));
    if (0 == numGroups)
    {
        n_error("nMemoryAnimation::LoadNax2(): File %s has no groups! Invalid Export ?", filename);
        File.Close();
        return false;
    }

    int numKeys;
	File.Read(&numKeys, sizeof(int));

    SetNumGroups(numGroups);
    keyArray.SetFixedSize(numKeys);

    // read groups
    int groupIndex = 0;
    for (groupIndex = 0; groupIndex < numGroups; groupIndex++)
    {
        int numCurves;
        int startKey;
        int numKeys;
        int keyStride;
        float keyTime;
        float fadeInFrames;
        int loopType;
		File.Read(&numCurves, sizeof(int));
		File.Read(&startKey, sizeof(int));
		File.Read(&numKeys, sizeof(int));
		File.Read(&keyStride, sizeof(int));
		File.Read(&keyTime, sizeof(int));
		File.Read(&fadeInFrames, sizeof(float));
		File.Read(&loopType, sizeof(int));

        Group& group = GetGroupAt(groupIndex);
        group.SetNumCurves(numCurves);
        group.SetStartKey(startKey);
        group.SetNumKeys(numKeys);
        group.SetKeyStride(keyStride);
        group.SetKeyTime(keyTime);
        group.SetFadeInFrames(fadeInFrames);
        group.SetLoopType((Group::LoopType) loopType);
    }

    // read curves
    for (groupIndex = 0; groupIndex < numGroups; groupIndex++)
    {
        Group& group = GetGroupAt(groupIndex);
        int numCurves = group.GetNumCurves();
        int curveIndex;
        for (curveIndex = 0; curveIndex < numCurves; curveIndex++)
        {
            static vector4 collapsedKey;
            int ipolType;
            int firstKeyIndex;
            int isAnim;
			File.Read(&ipolType, sizeof(int));
			File.Read(&firstKeyIndex, sizeof(int));
			File.Read(&isAnim, sizeof(int));
			File.Read(&collapsedKey.x, sizeof(float));
			File.Read(&collapsedKey.y, sizeof(float));
			File.Read(&collapsedKey.z, sizeof(float));
			File.Read(&collapsedKey.w, sizeof(float));

            Curve& curve = group.GetCurveAt(curveIndex);
            curve.SetIpolType((Curve::IpolType) ipolType);
            curve.SetConstValue(collapsedKey);
            curve.SetIsAnimated(isAnim);
            curve.SetFirstKeyIndex(firstKeyIndex);
        }
    }

    // read keys
    if (numKeys > 0)
    {
        int keyArraySize = numKeys * sizeof(vector4);
        File.Read(&(keyArray[0]), keyArraySize);
    }

    // cleanup
    File.Close();
    return true;
}

//------------------------------------------------------------------------------
/**
    Samples the current values for a number of curves in the given
    animation group. The sampled values will be written to a client provided
    vector4 array.

    @param  time                a point in time
    @param  groupIndex          index of animation group to sample from
    @param  firstCurveIndex     group-relative curve index of first curve to sample
    @param  numCurves           number of curves to sample
    @param  dstKeyArray         pointer to vector4 array with numCurves element which
                                will be filled with the results
    - 18-Oct-2004   floh    Fixed collapsed curve check (now checks if start key
                            is -1, instead of the curveIpolType). The curve ipol type
                            MUST be set to something sane even for collapsed curves,
                            because it's used at various places for deviding whether
                            quaternion-interpolation must be used instead of simple
                            linear interpolation!!
*/
void
nMemoryAnimation::SampleCurves(float time, int groupIndex, int firstCurveIndex, int numCurves, vector4* dstKeyArray)
{
    // convert the time into 2 global key indexes and an inbetween value
    const Group& group = GetGroupAt(groupIndex);
    int startKey = group.GetStartKey();
    double frameTime = startKey * group.GetKeyTime();
    int keyIndex[2];
    int startKeyIndex[2];
    float startInbetween;
    float inbetween;
    ///calculation for start values of the curves (time 0)
    group.TimeToIndex(0.0f, startKeyIndex[0], startKeyIndex[1], startInbetween);
    group.TimeToIndex(time, keyIndex[0], keyIndex[1], inbetween);

    int i;
    static quaternion q0;
    static quaternion q1;
    static quaternion q;
    int animCount = 0;
    for (i = 0; i < numCurves; i++)
    {
       Curve& curve = group.GetCurveAt(i + firstCurveIndex);

       if (curve.GetFirstKeyIndex() == -1)
       {
           // a collapsed curve
           dstKeyArray[i] = curve.GetConstValue();
           curve.SetStartValue(curve.GetConstValue());
       }
       else
       {
           switch (curve.GetIpolType())
           {
               case Curve::Step:
               {
                   int index0 = curve.GetFirstKeyIndex() + keyIndex[0];
                   dstKeyArray[i] = keyArray[index0];

                   index0 = curve.GetFirstKeyIndex();
                   curve.SetStartValue(keyArray[index0]);
               }
               break;

               case Curve::Quat:
               {
                   int curveFirstKeyIndex = curve.GetFirstKeyIndex();
                   int index0 = curveFirstKeyIndex + keyIndex[0];
                   int index1 = curveFirstKeyIndex + keyIndex[1];
                   q0.set(keyArray[index0].x, keyArray[index0].y, keyArray[index0].z, keyArray[index0].w);
                   q1.set(keyArray[index1].x, keyArray[index1].y, keyArray[index1].z, keyArray[index1].w);
                   q.slerp(q0, q1, inbetween);
                   dstKeyArray[i].set(q.x, q.y, q.z, q.w);

                   index0 = curveFirstKeyIndex + startKeyIndex[0];
                   index1 = curveFirstKeyIndex + startKeyIndex[1];
                   q0.set(keyArray[index0].x, keyArray[index0].y, keyArray[index0].z, keyArray[index0].w);
                   q1.set(keyArray[index1].x, keyArray[index1].y, keyArray[index1].z, keyArray[index1].w);
                   q.slerp(q0, q1, startInbetween);
                   vector4 val(q.x, q.y, q.z, q.w);
                   curve.SetStartValue(val);
               }
               break;

               case Curve::Linear:
               {
                   int curveFirstKeyIndex = curve.GetFirstKeyIndex();
                   int index0 = curveFirstKeyIndex + keyIndex[0];
                   int index1 = curveFirstKeyIndex + keyIndex[1];
                   const vector4& v0 = keyArray[index0];
                   const vector4& v1 = keyArray[index1];
                   dstKeyArray[i] = v0 + ((v1 - v0) * inbetween);

                   index0 = curveFirstKeyIndex + startKeyIndex[0];
                   index1 = curveFirstKeyIndex + startKeyIndex[1];
                   vector4& v2 = keyArray[index0];
                   vector4& v3 = keyArray[index1];
                   curve.SetStartValue(v2 + ((v3 - v2) * startInbetween));
               }
               break;

               default:
                   n_error("nMemoryAnimation::SampleCurves(): invalid curveIpolType %d!", curve.GetIpolType());
                   break;
           }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
int
nMemoryAnimation::GetByteSize()
{
    return keyArray.Size() * sizeof(vector4);
}
