#include "TransformSRT.h"

#include <StdDEM.h>

namespace Math
{

inline void GetDecompositionOrder(float Scales[3], UPTR& First, UPTR& Second, UPTR& Third)
{
	if (Scales[0] < Scales[1])   
	{    
		if (Scales[1] < Scales[2])    
		{
			First = 2;
			Second = 1;
			Third = 0;
		}
		else        
		{
			First = 1;

			if (Scales[0] < Scales[2])
			{       
				Second = 2; 
				Third = 0; 
			}       
			else    
			{       
				Second = 0; 
				Third = 2; 
			}       
		}
	}    
	else 
	{    
		if (Scales[0] < Scales[2])    
		{
			First = 2;
			Second = 0;
			Third = 1;
		}
		else        
		{
			First = 0;

			if (Scales[1] < Scales[2])
			{       
				Second = 2; 
				Third = 1; 
			}       
			else    
			{       
				Second = 1; 
				Third = 2; 
			}       
		}
	}
}
//---------------------------------------------------------------------

bool CTransformSRT::FromMatrix(const matrix44& Tfm)
{
	Translation = Tfm.Translation();

	static const vector3* pCanonicalBasis[3] = { &vector3::AxisX, &vector3::AxisY, &vector3::AxisZ };

	matrix44 Tmp(Tfm);
	Tmp.Translation().set(0.f, 0.f, 0.f);
	Tmp.m[3][3] = 1.f;

	vector3* pBasis[3];
	pBasis[0] = &Tmp.AxisX();
	pBasis[1] = &Tmp.AxisY();
	pBasis[2] = &Tmp.AxisZ();

	float* pScales = (float*)&Scale;

	pScales[0] = pBasis[0]->Length();
	pScales[1] = pBasis[1]->Length();
	pScales[2] = pBasis[2]->Length();

	UPTR a, b, c;
	GetDecompositionOrder(pScales, a, b, c);

	const float DECOMPOSE_EPSILON = 0.0001f;

	if (pScales[a] < DECOMPOSE_EPSILON) *pBasis[a] = *pCanonicalBasis[a];
	else pBasis[a]->norm();

	if (pScales[b] < DECOMPOSE_EPSILON)
	{
		float BasisAAbs[3];
		BasisAAbs[0] = n_fabs(pBasis[a]->x);
		BasisAAbs[1] = n_fabs(pBasis[a]->y);
		BasisAAbs[2] = n_fabs(pBasis[a]->z);

		UPTR aa, bb, cc;
		GetDecompositionOrder(BasisAAbs, aa, bb, cc);

		*pBasis[b] = (*pBasis[a]) * (*pCanonicalBasis[cc]); // Cross
	}

	pBasis[b]->norm();

	if (pScales[c] < DECOMPOSE_EPSILON)
		*pBasis[c] = (*pBasis[a]) * (*pBasis[b]); // Cross

	pBasis[c]->norm();

	float Det = Tmp.det();

	// Use Kramer's rule to check for handedness of coordinate system and switch it if needed
	if (Det < 0.0f)
	{
		pScales[a] = -pScales[a];
		(*pBasis[a]) *= -1.f;
		Det = -Det;
	}

	Det -= 1.f;
	Det *= Det;

	if (Det > DECOMPOSE_EPSILON) FAIL; // Non-SRT matrix encountered

	Rotation = Tmp.ToQuaternion();

	OK;
}
//---------------------------------------------------------------------

}
