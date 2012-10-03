#ifndef N_DEBUG_H
#define N_DEBUG_H

// Nebula debug macros.
// n_assert()  - the vanilla assert() Macro
// n_assert2() - an assert() plus a message from the programmer

#ifdef __NEBULA_NO_ASSERT__
	#define n_assert(exp)
	#define n_assert2(exp, msg)
	#define n_assert_dbg(exp)
	#define n_assert2_dbg(exp, msg)
	#define n_dxtrace(hr, msg)
#else
	#define n_assert(exp)				{ if (!(exp)) n_barf(#exp, __FILE__, __LINE__); }
	#define n_assert2(exp, msg)			{ if (!(exp)) n_barf2(#exp, msg, __FILE__, __LINE__); }

	#ifdef _DEBUG
		#define n_assert_dbg(exp)		n_assert(exp)
		#define n_assert2_dbg(exp, msg)	n_assert2(exp, msg)
	#else
		#define n_assert_dbg(exp)
		#define n_assert2_dbg(exp, msg)
	#endif

	// dx9 specific: check HRESULT and display DX9 specific message box
	#define n_dxtrace(hr, msg) { if (FAILED(hr)) DXTrace(__FILE__, __LINE__, hr, msg, true); }

#endif

#endif
