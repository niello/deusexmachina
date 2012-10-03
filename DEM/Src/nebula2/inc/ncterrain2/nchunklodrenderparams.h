#ifndef N_CHUNKLODRENDERPARAMS_H
#define N_CHUNKLODRENDERPARAMS_H
//------------------------------------------------------------------------------
/**
    @class nChunkLodRenderParams
    @ingroup NCTerrain2

    @brief Define render parameters for the ChunkLOD classes.
    
    (C) 2003 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "mathlib/matrix.h"

class nGfxServer2;
class nShader2;
class nChunkLodTree;

//------------------------------------------------------------------------------
struct nChunkLodRenderParams
{
    nChunkLodTree* tree;
    nGfxServer2* gfxServer;
    nShader2* shader;
    matrix44 modelViewProj;
    int numMeshesRendered;
    int numTexturesRendered;
};
//------------------------------------------------------------------------------
#endif    

