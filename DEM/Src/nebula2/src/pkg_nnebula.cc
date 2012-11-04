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
extern bool n_init_nscenenode (nClass *, nKernelServer *);
extern void *n_new_nscenenode (void);
extern bool n_init_nanimation (nClass *, nKernelServer *);
extern void *n_new_nanimation (void);
extern bool n_init_ninstancestream (nClass *, nKernelServer *);
extern void *n_new_ninstancestream (void);
extern bool n_init_nmesh2 (nClass *, nKernelServer *);
extern void *n_new_nmesh2 (void);
extern bool n_init_nmesharray (nClass *, nKernelServer *);
extern void *n_new_nmesharray (void);
extern bool n_init_nshader2 (nClass *, nKernelServer *);
extern void *n_new_nshader2 (void);
extern bool n_init_ntexture2 (nClass *, nKernelServer *);
extern void *n_new_ntexture2 (void);
extern bool n_init_ntransformnode (nClass *, nKernelServer *);
extern void *n_new_ntransformnode (void);
extern bool n_init_nmemoryanimation (nClass *, nKernelServer *);
extern void *n_new_nmemoryanimation (void);
extern bool n_init_nskinanimator (nClass *, nKernelServer *);
extern void *n_new_nskinanimator (void);
extern bool n_init_nabstractcameranode (nClass *, nKernelServer *);
extern void *n_new_nabstractcameranode (void);
extern bool n_init_nabstractshadernode (nClass *, nKernelServer *);
extern void *n_new_nabstractshadernode (void);
extern bool n_init_nlodnode (nClass *, nKernelServer *);
extern void *n_new_nlodnode (void);
extern bool n_init_nskynode (nClass *, nKernelServer *);
extern void *n_new_nskynode (void);
extern bool n_init_ncameranode (nClass *, nKernelServer *);
extern void *n_new_ncameranode (void);
extern bool n_init_nclippingcameranode (nClass *, nKernelServer *);
extern void *n_new_nclippingcameranode (void);
extern bool n_init_noverlookcameranode (nClass *, nKernelServer *);
extern void *n_new_noverlookcameranode (void);
extern bool n_init_nlightnode (nClass *, nKernelServer *);
extern void *n_new_nlightnode (void);
extern bool n_init_nmaterialnode (nClass *, nKernelServer *);
extern void *n_new_nmaterialnode (void);
extern bool n_init_nskystate (nClass *, nKernelServer *);
extern void *n_new_nskystate (void);
extern bool n_init_nreflectioncameranode (nClass *, nKernelServer *);
extern void *n_new_nreflectioncameranode (void);
extern bool n_init_nshapenode (nClass *, nKernelServer *);
extern void *n_new_nshapenode (void);
extern bool n_init_nparticleshapenode (nClass *, nKernelServer *);
extern void *n_new_nparticleshapenode (void);
extern bool n_init_nparticleshapenode2 (nClass *, nKernelServer *);
extern void *n_new_nparticleshapenode2 (void);
extern bool n_init_nskinshapenode (nClass *, nKernelServer *);
extern void *n_new_nskinshapenode (void);
extern bool n_init_nchunklodmesh (nClass *, nKernelServer *);
extern void *n_new_nchunklodmesh (void);
extern bool n_init_nchunklodtree (nClass *, nKernelServer *);
extern void *n_new_nchunklodtree (void);
extern bool n_init_nterrainnode (nClass *, nKernelServer *);
extern void *n_new_nterrainnode (void);

void nnebula()
{
    nKernelServer::Instance()->AddModule("nresource",
                                 n_init_nresource,
                                 n_new_nresource);
    nKernelServer::Instance()->AddModule("nscenenode",
                                 n_init_nscenenode,
                                 n_new_nscenenode);
    nKernelServer::Instance()->AddModule("nanimation",
                                 n_init_nanimation,
                                 n_new_nanimation);
    nKernelServer::Instance()->AddModule("ninstancestream",
                                 n_init_ninstancestream,
                                 n_new_ninstancestream);
    nKernelServer::Instance()->AddModule("nmesh2",
                                 n_init_nmesh2,
                                 n_new_nmesh2);
    nKernelServer::Instance()->AddModule("nmesharray",
                                 n_init_nmesharray,
                                 n_new_nmesharray);
    nKernelServer::Instance()->AddModule("nshader2",
                                 n_init_nshader2,
                                 n_new_nshader2);
    nKernelServer::Instance()->AddModule("ntexture2",
                                 n_init_ntexture2,
                                 n_new_ntexture2);
    nKernelServer::Instance()->AddModule("ntransformnode",
                                 n_init_ntransformnode,
                                 n_new_ntransformnode);
    nKernelServer::Instance()->AddModule("nmemoryanimation",
                                 n_init_nmemoryanimation,
                                 n_new_nmemoryanimation);
    nKernelServer::Instance()->AddModule("nskinanimator",
                                 n_init_nskinanimator,
                                 n_new_nskinanimator);
    nKernelServer::Instance()->AddModule("nabstractcameranode",
                                 n_init_nabstractcameranode,
                                 n_new_nabstractcameranode);
    nKernelServer::Instance()->AddModule("nabstractshadernode",
                                 n_init_nabstractshadernode,
                                 n_new_nabstractshadernode);
    nKernelServer::Instance()->AddModule("nlodnode",
                                 n_init_nlodnode,
                                 n_new_nlodnode);
    nKernelServer::Instance()->AddModule("nskynode",
                                 n_init_nskynode,
                                 n_new_nskynode);
    nKernelServer::Instance()->AddModule("ncameranode",
                                 n_init_ncameranode,
                                 n_new_ncameranode);
    nKernelServer::Instance()->AddModule("nclippingcameranode",
                                 n_init_nclippingcameranode,
                                 n_new_nclippingcameranode);
    nKernelServer::Instance()->AddModule("noverlookcameranode",
                                 n_init_noverlookcameranode,
                                 n_new_noverlookcameranode);
    nKernelServer::Instance()->AddModule("nlightnode",
                                 n_init_nlightnode,
                                 n_new_nlightnode);
    nKernelServer::Instance()->AddModule("nmaterialnode",
                                 n_init_nmaterialnode,
                                 n_new_nmaterialnode);
    nKernelServer::Instance()->AddModule("nskystate",
                                 n_init_nskystate,
                                 n_new_nskystate);
    nKernelServer::Instance()->AddModule("nreflectioncameranode",
                                 n_init_nreflectioncameranode,
                                 n_new_nreflectioncameranode);
    nKernelServer::Instance()->AddModule("nshapenode",
                                 n_init_nshapenode,
                                 n_new_nshapenode);
    nKernelServer::Instance()->AddModule("nparticleshapenode",
                                 n_init_nparticleshapenode,
                                 n_new_nparticleshapenode);
    nKernelServer::Instance()->AddModule("nparticleshapenode2",
                                 n_init_nparticleshapenode2,
                                 n_new_nparticleshapenode2);
    nKernelServer::Instance()->AddModule("nskinshapenode",
                                 n_init_nskinshapenode,
                                 n_new_nskinshapenode);
    nKernelServer::Instance()->AddModule("nchunklodmesh",
                                 n_init_nchunklodmesh,
                                 n_new_nchunklodmesh);
    nKernelServer::Instance()->AddModule("nchunklodtree",
                                 n_init_nchunklodtree,
                                 n_new_nchunklodtree);
    nKernelServer::Instance()->AddModule("nterrainnode",
                                 n_init_nterrainnode,
                                 n_new_nterrainnode);
}

//-----------------------------------------------------------------------------
// EOF
//-----------------------------------------------------------------------------

