//------------------------------------------------------------------------------
//  nrpsection.cc
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "renderpath/nrpsection.h"

//------------------------------------------------------------------------------
/**
*/
nRpSection::nRpSection() :
    renderPath(0),
    inBegin(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nRpSection::~nRpSection()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Validate the section object. This will invoke Validate() on all owned pass objects.
*/
void
nRpSection::Validate()
{
    n_assert(this->renderPath);

    // setup profiler
    #if __NEBULA_STATS__
    if (!this->prof.IsValid())
    {
        nString n;
        n.Format("profRpSection_%s", this->name.Get());
        this->prof.Initialize(n.Get());
    }
    #endif

    // invoke validate on passes
    int i;
    int num = this->passes.Size();
    for (i = 0; i < num; i++)
    {
        this->passes[i].SetRenderPath(this->renderPath);
    #if __NEBULA_STATS__
        this->passes[i].SetSection(this);
    #endif
        this->passes[i].Validate();
    }
}

//------------------------------------------------------------------------------
/**
    Begin rendering the section. This will validate all embedded objects.
    Returns the number of scene passes in the section.
    After begin, each pass should be "rendered" recursively.
*/
int
nRpSection::Begin()
{
    n_assert(!this->inBegin);

    #if __NEBULA_STATS__
    this->prof.Start();
    #endif

    this->Validate();
    this->inBegin = true;
    return this->passes.Size();
}

//------------------------------------------------------------------------------
/**
    Finish rendering the render path.
*/
void
nRpSection::End()
{
    n_assert(this->inBegin);
    this->inBegin = false;

    #if __NEBULA_STATS__
    this->prof.Stop();
    #endif
}

