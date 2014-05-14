#include "Matrix44.h"

const matrix44 matrix44::Identity = matrix44(1.0f, 0.0f, 0.0f, 0.0f,
											 0.0f, 1.0f, 0.0f, 0.0f,
											 0.0f, 0.0f, 1.0f, 0.0f,
											 0.0f, 0.0f, 0.0f, 1.0f);

const matrix44 matrix44::Ortho = matrix44(0.5f, 0.0f,    0.0f, 0.0f,
                                          0.0f, 0.6667f, 0.0f, 0.0f,
                                          0.0f, 0.0f,   -0.5f, 0.0f,
                                          0.0f, 0.0f,    0.5f, 1.0f);
