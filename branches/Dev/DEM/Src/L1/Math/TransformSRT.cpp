#include "TransformSRT.h"

#include <StdDEM.h>

namespace Math
{

inline void RankDecompose(float Scales[3], DWORD& First, DWORD& Second, DWORD& Third)
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
	Translation = Tfm.pos_component();

	//!!!TO static V3 fields!
	vector3 VX(1.f, 0.f, 0.f);
	vector3 VY(0.f, 1.f, 0.f);
	vector3 VZ(0.f, 0.f, 1.f);
	static vector3* pCanonicalBasis[3] = { &VX, &VY, &VZ };

	vector3* pBasis[3];
	pBasis[0] = &Tfm.x_component();
	pBasis[1] = &Tfm.y_component();
	pBasis[2] = &Tfm.z_component();

	matrix44 Tmp(Tfm);
	Tmp.pos_component().set(0.f, 0.f, 0.f);
	Tmp.m[3][3] = 1.f;

	float* pScales = (float*)&Scale;

	pScales[0] = pBasis[0]->len();
	pScales[1] = pBasis[1]->len();
	pScales[2] = pBasis[2]->len();

	DWORD a, b, c;
	RankDecompose(pScales, a, b, c);

	const float DECOMPOSE_EPSILON = 0.0001f;

	if (pScales[a] < DECOMPOSE_EPSILON) pBasis[a] = pCanonicalBasis[a];
	else pBasis[a]->norm();

	if (pScales[b] < DECOMPOSE_EPSILON)
	{
		FLOAT BasisAAbs[3];
		BasisAAbs[0] = n_fabs(pBasis[a]->x);
		BasisAAbs[1] = n_fabs(pBasis[a]->y);
		BasisAAbs[2] = n_fabs(pBasis[a]->z);

		DWORD aa, bb, cc;
		RankDecompose(BasisAAbs, aa, bb, cc);

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
		(*pBasis[a]) *= -1;
		Det = -Det;
	}

	Det -= 1.0f;
	Det *= Det;

	if (Det > DECOMPOSE_EPSILON) FAIL; // Non-SRT matrix encountered

	Rotation = Tmp.get_quaternion();

	OK;
}
//---------------------------------------------------------------------

}
