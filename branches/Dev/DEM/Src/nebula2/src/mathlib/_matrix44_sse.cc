#if defined(__USE_SSE__) || defined(DOXYGEN)

#include "mathlib/_matrix44_sse.h"

const _matrix44_sse _matrix44_sse::identity = _matrix44_sse();

const _matrix44_sse _matrix44_sse::ortho = _matrix44_sse(0.5f, 0.0f,    0.0f, 0.0f,
                                                         0.0f, 0.6667f, 0.0f, 0.0f,
                                                         0.0f, 0.0f,   -0.5f, 0.0f,
                                                         0.0f, 0.0f,    0.5f, 1.0f);

#endif // defined(__USE_SSE__) || defined(DOXYGEN)
