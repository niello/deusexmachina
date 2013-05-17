#include "mathlib/_vector3.h"

#include <mathlib/_vector4.h>

const _vector3 _vector3::Zero(0.f, 0.f, 0.f);
const _vector3 _vector3::One(1.f, 1.f, 1.f);
const _vector3 _vector3::Up(0.f, 1.f, 0.f);
const _vector3 _vector3::AxisX(1.f, 0.f, 0.f);
const _vector3 _vector3::AxisY(0.f, 1.f, 0.f);
const _vector3 _vector3::AxisZ(0.f, 0.f, 1.f);

_vector3::_vector3(const _vector4& vec): x(vec.x), y(vec.y), z(vec.z)
{
}
//---------------------------------------------------------------------
