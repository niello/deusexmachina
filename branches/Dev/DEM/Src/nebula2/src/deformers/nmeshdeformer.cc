//------------------------------------------------------------------------------
//  nmeshdeformer.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "deformers/nmeshdeformer.h"

//------------------------------------------------------------------------------
/**
*/
nMeshDeformer::nMeshDeformer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nMeshDeformer::~nMeshDeformer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Perform the actual mesh deformation. Override this method in a subclass.
*/
void
nMeshDeformer::Compute()
{
    // empty
}
