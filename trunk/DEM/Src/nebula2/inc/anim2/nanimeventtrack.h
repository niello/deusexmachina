#ifndef N_ANIMEVENTTRACK_H
#define N_ANIMEVENTTRACK_H
//------------------------------------------------------------------------------
/**
    @class nAnimEventTrack
    @ingroup Anim2
    @brief An anim event track contains timed events which are associated
    with a matrix. These events are triggered when the playback cursor
    of an anim clip goes over an anim event. Anim event tracks are
    usually added to anim clips.

    (C) 2005 Radon Labs GmbH
*/
#include "kernel/ntypes.h"
#include "util/nfixedarray.h"
#include "anim2/nanimevent.h"
#include "util/nstring.h"

//------------------------------------------------------------------------------
class nAnimEventTrack
{
public:
    /// constructor
    nAnimEventTrack();
    /// set the track's name
    void SetName(const nString& n);
    /// get the tracks name
    const nString& GetName() const;
    /// set number of events
    void SetNumEvents(int num);
    /// get number of events
    int GetNumEvents() const;
    /// set an event at given index
    void SetEvent(int index, const nAnimEvent& e);
    /// add an event at the end
    void AddEvent(const nAnimEvent& e);
    /// get event at index
    const nAnimEvent& GetEvent(int index) const;
    /// read/write access to animation event array
    nArray<nAnimEvent>& EventArray();

private:
    nString name;
    nArray<nAnimEvent> eventArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nAnimEventTrack::nAnimEventTrack()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimEventTrack::SetName(const nString& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nAnimEventTrack::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimEventTrack::SetNumEvents(int num)
{
    this->eventArray.SetFixedSize(num);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimEventTrack::GetNumEvents() const
{
    return this->eventArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimEventTrack::SetEvent(int index, const nAnimEvent& event)
{
    this->eventArray[index] = event;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimEventTrack::AddEvent(const nAnimEvent& event)
{
    this->eventArray.Append(event);
}

//------------------------------------------------------------------------------
/**
*/
inline
const nAnimEvent&
nAnimEventTrack::GetEvent(int index) const
{
    return this->eventArray[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
nArray<nAnimEvent>&
nAnimEventTrack::EventArray()
{
    return this->eventArray;
}

//------------------------------------------------------------------------------
#endif
