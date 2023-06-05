#include "AABB.h"

const CAABB CAABB::Empty(vector3(0.f, 0.f, 0.f), vector3(0.f, 0.f, 0.f));
const CAABB CAABB::Invalid(vector3(0.f, 0.f, 0.f), vector3(-1.f, -1.f, -1.f));

float CAABB::SqDistance(const vector3& Point) const
{
	float SqDist = 0.f;
	for (U32 i = 0; i < 3; ++i)
	{
		const float PointN = Point.v[i];
		const float MinN = Min.v[i];
		if (PointN < MinN)
		{
			const float DistN = MinN - PointN;
			SqDist += DistN * DistN;
		}
		else
		{
			const float MaxN = Max.v[i];
			if (PointN > MaxN)
			{
				const float DistN = PointN - MaxN;
				SqDist += DistN * DistN;
			}
		}
	}
	return SqDist;
}
//---------------------------------------------------------------------
