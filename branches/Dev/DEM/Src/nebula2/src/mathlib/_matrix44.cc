#include "mathlib/_matrix44.h"

const _matrix44 _matrix44::identity = _matrix44(1.0f, 0.0f, 0.0f, 0.0f,
												0.0f, 1.0f, 0.0f, 0.0f,
												0.0f, 0.0f, 1.0f, 0.0f,
												0.0f, 0.0f, 0.0f, 1.0f);

const _matrix44 _matrix44::ortho = _matrix44(0.5f, 0.0f,    0.0f, 0.0f,
                                             0.0f, 0.6667f, 0.0f, 0.0f,
                                             0.0f, 0.0f,   -0.5f, 0.0f,
                                             0.0f, 0.0f,    0.5f, 1.0f);
