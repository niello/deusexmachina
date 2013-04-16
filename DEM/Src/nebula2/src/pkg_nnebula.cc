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

void nnebula()
{
    nKernelServer::Instance()->AddModule("nresource",
                                 n_init_nresource,
                                 n_new_nresource);
    nKernelServer::Instance()->AddModule("nmesh2",
                                 n_init_nmesh2,
                                 n_new_nmesh2);
    nKernelServer::Instance()->AddModule("nshader2",
                                 n_init_nshader2,
                                 n_new_nshader2);
    nKernelServer::Instance()->AddModule("ntexture2",
                                 n_init_ntexture2,
                                 n_new_ntexture2);
}

//-----------------------------------------------------------------------------
// EOF
//-----------------------------------------------------------------------------

