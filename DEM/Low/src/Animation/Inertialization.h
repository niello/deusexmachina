#pragma once
#include <Data/FixedArray.h>
#include <acl/math/vector4_32.h>

// Inertialization blending
// See:
// https://www.gdcvault.com/play/1025165/Inertialization
// https://www.gdcvault.com/play/1025331/Inertialization

namespace DEM::Anim
{
class CPoseBuffer;

class CInertializationPoseDiff final
{
protected:

	struct CQuinticCurve4
	{
		acl::Vector4_32 _a;
		acl::Vector4_32 _b;
		acl::Vector4_32 _c;
		acl::Vector4_32 _d;
		acl::Vector4_32 _v0;
		acl::Vector4_32 _x0;
		acl::Vector4_32 _sign;

		void Prepare(acl::Vector4_32 x0, acl::Vector4_32 v0, acl::Vector4_32 Duration);

		//!!!TODO PERF:can use mm_xor_ps with signed zeros in _sign! Then must vector_select x0 and v0 in Prepare()!
		DEM_FORCE_INLINE acl::Vector4_32 Evaluate(acl::Vector4_32Arg0 t) const
		{
			auto Result = acl::vector_mul_add(_a, t, _b);
			Result = acl::vector_mul_add(Result, t, _c);
			Result = acl::vector_mul_add(Result, t, _d);
			Result = acl::vector_mul_add(Result, t, _v0);
			Result = acl::vector_mul_add(Result, t, _x0);
			return acl::vector_mul(Result, _sign);
		}
	};

	//???store BoneDiff4? 4 scale axes etc. When iterate, must correctly handle last 0-3 elements
	struct CBoneDiff
	{
		acl::Vector4_32 ScaleAxis;
		acl::Vector4_32 RotationAxis;
		acl::Vector4_32 TranslationDir;

		DEM_ALLOCATE_ALIGNED(alignof(CBoneDiff));
	};

	struct CBoneCurves
	{
		CQuinticCurve4 ScaleParams;
		CQuinticCurve4 RotationParams;
		CQuinticCurve4 TranslationParams;

		DEM_ALLOCATE_ALIGNED(alignof(CBoneDiff));
	};

	CFixedArray<CBoneDiff>   _BoneDiffs; //!!!???rename to bases!?
	CFixedArray<CBoneCurves> _Curves;

public:

	void Init(const CPoseBuffer& CurrPose, const CPoseBuffer& PrevPose1, const CPoseBuffer& PrevPose2, float dt, float Duration);
	void ApplyTo(CPoseBuffer& Target, float ElapsedTime) const;
};

}
