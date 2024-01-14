#include "Math.h"
#include <Math/WELL512.h>

namespace Math
{
static CWELL512 DefaultRNG;

U32 RandomU32()
{
	return DefaultRNG();
}
//---------------------------------------------------------------------

}
