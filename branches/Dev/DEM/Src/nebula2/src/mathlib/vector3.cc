#include "mathlib/vector3.h"

#include <mathlib/vector4.h>

const vector3 vector3::Zero(0.f, 0.f, 0.f);
const vector3 vector3::One(1.f, 1.f, 1.f);
const vector3 vector3::Up(0.f, 1.f, 0.f);
const vector3 vector3::AxisX(1.f, 0.f, 0.f);
const vector3 vector3::AxisY(0.f, 1.f, 0.f);
const vector3 vector3::AxisZ(0.f, 0.f, 1.f);
const vector3 vector3::BaseDir(0.f, 0.f, -1.f);

vector3::vector3(const vector4& vec): x(vec.x), y(vec.y), z(vec.z)
{
}
//---------------------------------------------------------------------
