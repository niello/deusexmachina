#include "mathlib/matrix44.h"

const matrix44 matrix44::identity = matrix44(1.0f, 0.0f, 0.0f, 0.0f,
												0.0f, 1.0f, 0.0f, 0.0f,
												0.0f, 0.0f, 1.0f, 0.0f,
												0.0f, 0.0f, 0.0f, 1.0f);

const matrix44 matrix44::ortho = matrix44(0.5f, 0.0f,    0.0f, 0.0f,
                                             0.0f, 0.6667f, 0.0f, 0.0f,
                                             0.0f, 0.0f,   -0.5f, 0.0f,
                                             0.0f, 0.0f,    0.5f, 1.0f);
