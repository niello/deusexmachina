//-----------------------------------------------------------------------------
// pkg_ndirect3d9.cc
// MACHINE GENERATED FROM
// code\nebula2\bldfiles\nebula2libs.bld
// DON'T EDIT!
//-----------------------------------------------------------------------------
#include "kernel/ntypes.h"
#include "kernel/nkernelserver.h"
#ifdef __XBxX__
#undef __WIN32__
#endif

extern "C" void ndirect3d9();

#ifdef __WIN32__
extern bool n_init_nd3d9mesh (nClass *, nKernelServer *);
extern void *n_new_nd3d9mesh (void);
#endif //__WIN32__

#ifdef __WIN32__
extern bool n_init_nd3d9mesharray (nClass *, nKernelServer *);
extern void *n_new_nd3d9mesharray (void);
#endif //__WIN32__

#ifdef __WIN32__
extern bool n_init_nd3d9shader (nClass *, nKernelServer *);
extern void *n_new_nd3d9shader (void);
#endif //__WIN32__

#ifdef __WIN32__
extern bool n_init_nd3d9texture (nClass *, nKernelServer *);
extern void *n_new_nd3d9texture (void);
#endif //__WIN32__


void ndirect3d9()
{
#ifdef __WIN32__
    nKernelServer::Instance()->AddModule("nd3d9mesh",
                                 n_init_nd3d9mesh,
                                 n_new_nd3d9mesh);
#endif //__WIN32__
#ifdef __WIN32__
    nKernelServer::Instance()->AddModule("nd3d9mesharray",
                                 n_init_nd3d9mesharray,
                                 n_new_nd3d9mesharray);
#endif //__WIN32__
#ifdef __WIN32__
    nKernelServer::Instance()->AddModule("nd3d9shader",
                                 n_init_nd3d9shader,
                                 n_new_nd3d9shader);
#endif //__WIN32__
#ifdef __WIN32__
    nKernelServer::Instance()->AddModule("nd3d9texture",
                                 n_init_nd3d9texture,
                                 n_new_nd3d9texture);
#endif //__WIN32__
}

//-----------------------------------------------------------------------------
// EOF
//-----------------------------------------------------------------------------

