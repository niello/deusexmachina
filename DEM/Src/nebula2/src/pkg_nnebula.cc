//-----------------------------------------------------------------------------
// pkg_nnebula.cc
// MACHINE GENERATED FROM
// code\nebula2\bldfiles\nebula2libs.bld
// DON'T EDIT!
//-----------------------------------------------------------------------------
#include "kernel/ntypes.h"
#include "kernel/nkernelserver.h"
#ifdef __XBxX__
#undef __WIN32__
#endif

extern "C" void nnebula();

extern bool n_init_nresource (nClass *, nKernelServer *);
extern void *n_new_nresource (void);
extern bool n_init_nmesh2 (nClass *, nKernelServer *);
extern void *n_new_nmesh2 (void);
extern bool n_init_nshader2 (nClass *, nKernelServer *);
extern void *n_new_nshader2 (void);
extern bool n_init_ntexture2 (nClass *, nKernelServer *);
extern void *n_new_ntexture2 (void);
extern bool n_init_nscenenode (nClass *, nKernelServer *);
extern void *n_new_nscenenode (void);
extern bool n_init_ntransformnode (nClass *, nKernelServer *);
extern void *n_new_ntransformnode (void);
extern bool n_init_nskynode (nClass *, nKernelServer *);
extern void *n_new_nskynode (void);
extern bool n_init_nlightnode (nClass *, nKernelServer *);
extern void *n_new_nlightnode (void);
extern bool n_init_nmaterialnode (nClass *, nKernelServer *);
extern void *n_new_nmaterialnode (void);
extern bool n_init_nskystate (nClass *, nKernelServer *);
extern void *n_new_nskystate (void);
extern bool n_init_nshapenode (nClass *, nKernelServer *);
extern void *n_new_nshapenode (void);

void nnebula()
{
    nKernelServer::Instance()->AddModule("nresource",
                                 n_init_nresource,
                                 n_new_nresource);
    nKernelServer::Instance()->AddModule("nscenenode",
                                 n_init_nscenenode,
                                 n_new_nscenenode);
    nKernelServer::Instance()->AddModule("nmesh2",
                                 n_init_nmesh2,
                                 n_new_nmesh2);
    nKernelServer::Instance()->AddModule("nshader2",
                                 n_init_nshader2,
                                 n_new_nshader2);
    nKernelServer::Instance()->AddModule("ntexture2",
                                 n_init_ntexture2,
                                 n_new_ntexture2);
    nKernelServer::Instance()->AddModule("ntransformnode",
                                 n_init_ntransformnode,
                                 n_new_ntransformnode);
    nKernelServer::Instance()->AddModule("nskynode",
                                 n_init_nskynode,
                                 n_new_nskynode);
    nKernelServer::Instance()->AddModule("nlightnode",
                                 n_init_nlightnode,
                                 n_new_nlightnode);
    nKernelServer::Instance()->AddModule("nmaterialnode",
                                 n_init_nmaterialnode,
                                 n_new_nmaterialnode);
    nKernelServer::Instance()->AddModule("nskystate",
                                 n_init_nskystate,
                                 n_new_nskystate);
    nKernelServer::Instance()->AddModule("nshapenode",
                                 n_init_nshapenode,
                                 n_new_nshapenode);
}

//-----------------------------------------------------------------------------
// EOF
//-----------------------------------------------------------------------------

