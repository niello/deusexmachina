#include "BlendSpace2D.h"
#include <Animation/AnimationController.h>
#include <Animation/SkeletonInfo.h>
#include <Math/DelaunayTriangulation.hpp>

namespace DEM::Math
{

template<>
struct CDelaunayInputTraits<DEM::Anim::CBlendSpace2D::CSample>
{
	static vector2 GetPoint(const DEM::Anim::CBlendSpace2D::CSample& Value) { return { Value.XValue, Value.YValue }; }
};

}

namespace DEM::Anim
{

CBlendSpace2D::CBlendSpace2D(CStrID XParamID, CStrID YParamID)
	: _XParamID(XParamID)
	, _YParamID(YParamID)
{
}
//---------------------------------------------------------------------

void CBlendSpace2D::Init(CAnimationInitContext& Context)
{
	EParamType XType, YType;
	if (!Context.Controller.FindParam(_XParamID, &XType, &_XParamIndex) || XType != EParamType::Float)
		_XParamIndex = INVALID_INDEX;
	if (!Context.Controller.FindParam(_YParamID, &YType, &_YParamIndex) || YType != EParamType::Float)
		_YParamIndex = INVALID_INDEX;

	if (!_Samples.empty())
	{
		// TODO: can obtain adjacency info as a side-effect of Delaunay triangulation?
		//!!!it may even speed up Delaunay triangulation from O(n^2) to O(n log n)!
		std::vector<std::array<uint32_t, 3>> Triangles;
		Math::Delaunay2D(_Samples.cbegin(), _Samples.cend(), Triangles);

		//!!!process degenerate cases (point, collinear segments)!

		// TODO: could calculate geometric median if it improves runtime search, but for now considered it an overkill
		vector2 Center;
		for (const auto& Sample : _Samples)
		{
			Center.x += Sample.XValue;
			Center.y += Sample.YValue;
		}
		Center /= static_cast<float>(_Samples.size());
		size_t StartIndex = INVALID_INDEX;

		const size_t TriCount = Triangles.size();
		_Triangles.resize(TriCount);
		for (size_t i = 0; i < TriCount; ++i)
		{
			const auto& SrcTri = Triangles[i];
			auto& Tri = _Triangles[i];
			Tri.Samples[vA] = &_Samples[SrcTri[vA]];
			Tri.Samples[vB] = &_Samples[SrcTri[vB]];
			Tri.Samples[vC] = &_Samples[SrcTri[vC]];

			// Cache constants for barycentric coordinate evaluation
			Tri.ax = Tri.Samples[vA]->XValue;
			Tri.ay = Tri.Samples[vA]->YValue;
			Tri.abx = Tri.Samples[vB]->XValue - Tri.ax;
			Tri.aby = Tri.Samples[vB]->YValue - Tri.ay;
			Tri.acx = Tri.Samples[vC]->XValue - Tri.ax;
			Tri.acy = Tri.Samples[vC]->YValue - Tri.ay;
			Tri.InvDenominator = 1.f / (Tri.abx * Tri.acy - Tri.acx * Tri.aby);

			// Check if this triangle contains the starting point
			if (StartIndex == INVALID_INDEX)
			{
				const float apx = Center.x - Tri.ax;
				const float apy = Center.y - Tri.ay;
				const float v = (apx * Tri.acy - Tri.acx * apy) * Tri.InvDenominator;
				if (v >= 0.f && v <= 1.f)
				{
					const float w = (Tri.abx * apy - apx * Tri.aby) * Tri.InvDenominator;
					if (w >= 0.f && (v + w) <= 1.f) StartIndex = i;
				}
			}
		}

		// Ensure the starting triangle is the first
		// TODO: could sort by adjacency to improve cache locality when searching. Worth it?
		n_assert(StartIndex != INVALID_INDEX);
		if (StartIndex != 0)
		{
			std::swap(Triangles[StartIndex], Triangles[0]);
			std::swap(_Triangles[StartIndex], _Triangles[0]);
		}

		// Build adjacency info. Only one edge can be common for two triangles.
		for (size_t i = 0; i < TriCount - 1; ++i)
		{
			const auto& SrcTri1 = Triangles[i];
			for (size_t j = i + 1; j < TriCount; ++j)
			{
				// TODO: could skip already filled edges. Note that only one edge can be common, early exits will be needed.
				uint32_t Edge1, Edge2;
				const auto& SrcTri2 = Triangles[j];
				/**/ if (SrcTri1[vA] == SrcTri2[vB] && SrcTri1[vB] == SrcTri2[vA]) { Edge1 = eAB; Edge2 = eAB; }
				else if (SrcTri1[vA] == SrcTri2[vC] && SrcTri1[vB] == SrcTri2[vB]) { Edge1 = eAB; Edge2 = eBC; }
				else if (SrcTri1[vA] == SrcTri2[vA] && SrcTri1[vB] == SrcTri2[vC]) { Edge1 = eAB; Edge2 = eCA; }
				else if (SrcTri1[vB] == SrcTri2[vB] && SrcTri1[vC] == SrcTri2[vA]) { Edge1 = eBC; Edge2 = eAB; }
				else if (SrcTri1[vB] == SrcTri2[vC] && SrcTri1[vC] == SrcTri2[vB]) { Edge1 = eBC; Edge2 = eBC; }
				else if (SrcTri1[vB] == SrcTri2[vA] && SrcTri1[vC] == SrcTri2[vC]) { Edge1 = eBC; Edge2 = eCA; }
				else if (SrcTri1[vC] == SrcTri2[vB] && SrcTri1[vA] == SrcTri2[vA]) { Edge1 = eCA; Edge2 = eAB; }
				else if (SrcTri1[vC] == SrcTri2[vC] && SrcTri1[vA] == SrcTri2[vB]) { Edge1 = eCA; Edge2 = eBC; }
				else if (SrcTri1[vC] == SrcTri2[vA] && SrcTri1[vA] == SrcTri2[vC]) { Edge1 = eCA; Edge2 = eCA; }
				else continue;

				_Triangles[i].Adjacent[Edge1] = j;
				_Triangles[j].Adjacent[Edge2] = i;
			}
		}

		// Map contour edges to a separate array with contour adjacency info
		for (size_t i = 0; i < TriCount; ++i)
		{
			auto& Tri = _Triangles[i];
			if (Tri.Adjacent[eBC] == INVALID_INDEX)
			{
				Tri.Adjacent[eBC] = TriCount + _Contour.size();
				_Contour.push_back({ i, eBC });
			}
			if (Tri.Adjacent[eCA] == INVALID_INDEX)
			{
				Tri.Adjacent[eCA] = TriCount + _Contour.size();
				_Contour.push_back({ i, eCA });
			}
			if (Tri.Adjacent[eAB] == INVALID_INDEX)
			{
				Tri.Adjacent[eAB] = TriCount + _Contour.size();
				_Contour.push_back({ i, eAB });
			}
		}

		// Build contour adjacency info
		const auto ContourSize = _Contour.size();
		for (size_t i = 0; i < ContourSize - 1; ++i)
		{
			const auto& Edge1 = _Contour[i];
			const auto& Tri1 = _Triangles[Edge1.TriIndex];
			const CSample* pEdge1SampleA = nullptr;
			const CSample* pEdge1SampleB = nullptr;
			switch (Edge1.EdgeIndex)
			{
				case eAB: pEdge1SampleA = Tri1.Samples[vA]; pEdge1SampleB = Tri1.Samples[vB]; break;
				case eBC: pEdge1SampleA = Tri1.Samples[vB]; pEdge1SampleB = Tri1.Samples[vC]; break;
				case eCA: pEdge1SampleA = Tri1.Samples[vC]; pEdge1SampleB = Tri1.Samples[vA]; break;
			}

			for (size_t j = i + 1; j < ContourSize; ++j)
			{
				const auto& Edge2 = _Contour[j];
				const auto& Tri2 = _Triangles[Edge2.TriIndex];
				const CSample* pEdge2SampleA = nullptr;
				const CSample* pEdge2SampleB = nullptr;
				switch (Edge2.EdgeIndex)
				{
					case eAB: pEdge2SampleA = Tri2.Samples[vA]; pEdge2SampleB = Tri2.Samples[vB]; break;
					case eBC: pEdge2SampleA = Tri2.Samples[vB]; pEdge2SampleB = Tri2.Samples[vC]; break;
					case eCA: pEdge2SampleA = Tri2.Samples[vC]; pEdge2SampleB = Tri2.Samples[vA]; break;
				}

				if (pEdge1SampleA == pEdge2SampleB)
				{
					_Contour[i].Adjacent[vA] = j;
					_Contour[j].Adjacent[vB] = i;
				}
				else if (pEdge1SampleB == pEdge2SampleA)
				{
					_Contour[i].Adjacent[vB] = j;
					_Contour[j].Adjacent[vA] = i;
				}
			}
		}

		// Initialize sample subgraphs
		for (auto& Sample : _Samples)
			Sample.Source->Init(Context);

		// Call after initializing sources, letting them to contribute to SkeletonInfo
		if (Context.SkeletonInfo && _Samples.size() > 1)
			_TmpPose.SetSize(Context.SkeletonInfo->GetNodeCount());
	}
}
//---------------------------------------------------------------------

//!!!DUPLICATED CODE! See 1D!
static inline void AdvanceNormalizedTime(float dt, float AnimLength, float& NormalizedTime)
{
	if (AnimLength)
	{
		NormalizedTime += (dt / AnimLength);
		if (NormalizedTime < 0.f) NormalizedTime += (1.f - std::truncf(NormalizedTime));
		else if (NormalizedTime > 1.f) NormalizedTime -= std::truncf(NormalizedTime);
	}
	else NormalizedTime = 0.f;
}
//---------------------------------------------------------------------

void CBlendSpace2D::Update(CAnimationUpdateContext& Context, float dt)
{
	if (_Samples.empty()) return;

	if (_Samples.size() == 1)
	{
		//???!!!after fixind dt due to scaling based on distance from input to sample?! See 1D.
		_Samples[0].Source->Update(Context, dt);
		return;
	}

	const float XInput = Context.Controller.GetFloat(_XParamIndex);
	const float YInput = Context.Controller.GetFloat(_YParamIndex);

	//???use cache if input parameter didn't change?

	//???TODO: optionally filter input to make transitions smoother?
	// then dt *= (BeforeFilter / AfterFilter);

	// Find triangle or segment of active samples
	// FIXME: support degenerate cases - point or collinear!
	float u, v, w;
	size_t CurrTriIndex = 0;
	const auto TriCount = _Triangles.size();
	while (true)
	{
		const CTriangle* pTri = &_Triangles[CurrTriIndex];
		const float apx = XInput - pTri->ax;
		const float apy = YInput - pTri->ay;
		v = (apx * pTri->acy - pTri->acx * apy) * pTri->InvDenominator;
		w = (pTri->abx * apy - apx * pTri->aby) * pTri->InvDenominator;
		u = 1.f - v - w;
		const auto NegativeMask = (static_cast<int>(w < 0.f) << 2) | (static_cast<int>(v < 0.f) << 1) | static_cast<int>(u < 0.f);

		// Zero mask means that all weights are positive and we are inside a triangle
		if (!NegativeMask)
		{
			_pActiveSamples[0] = pTri->Samples[0]->Source.get();
			_pActiveSamples[1] = pTri->Samples[1]->Source.get();
			_pActiveSamples[2] = pTri->Samples[2]->Source.get();
			break;
		}

		size_t EdgeIndex = eBC;
		switch (NegativeMask)
		{
			case 1: EdgeIndex = eBC; break;                 // Only A weight is negative, goto BC
			case 2: EdgeIndex = eCA; break;                 // Only B weight is negative, goto CA
			case 3: EdgeIndex = (v > u) ? eCA : eBC; break; // A and B weights are negative, goto BC/CA
			case 4: EdgeIndex = eAB; break;                 // Only C weight is negative, goto AB
			case 5: EdgeIndex = (w > u) ? eAB : eBC; break; // A and C weights are negative, goto BC/AB
			case 6: EdgeIndex = (w > v) ? eAB : eCA; break; // B and C weights are negative, goto CA/AB
		}

		// If adjacent triangle exists, proceed to it
		if (pTri->Adjacent[EdgeIndex] < TriCount)
		{
			CurrTriIndex = pTri->Adjacent[EdgeIndex];
			continue;
		}

		// We moved outside the convex poly, project input onto its contour
		UPTR ContourWalkDirection = vC; // vA and vB are valid, vC is for undefined
		const CEdge* pContourEdge = &_Contour[pTri->Adjacent[EdgeIndex] - TriCount];
		n_assert_dbg(pContourEdge->TriIndex == CurrTriIndex && pContourEdge->EdgeIndex == EdgeIndex);
		do
		{
			switch (EdgeIndex)
			{
				case eAB:
				{
					v = (apx * pTri->abx + apy * pTri->aby) / (pTri->abx * pTri->abx + pTri->aby * pTri->aby);
					_pActiveSamples[0] = pTri->Samples[0]->Source.get();
					_pActiveSamples[1] = pTri->Samples[1]->Source.get();
					break;
				}
				case eBC:
				{
					// We do not cache this because it adds 2 floats to every triangle
					// but is useful only for triangles with BC being a border edge
					const float bcx = pTri->Samples[2]->XValue - pTri->Samples[1]->XValue;
					const float bcy = pTri->Samples[2]->YValue - pTri->Samples[1]->YValue;
					const float bpx = XInput - pTri->Samples[1]->XValue;
					const float bpy = YInput - pTri->Samples[1]->YValue;

					v = (bpx * bcx + bpy * bcy) / (bcx * bcx + bcy * bcy);
					_pActiveSamples[0] = pTri->Samples[1]->Source.get();
					_pActiveSamples[1] = pTri->Samples[2]->Source.get();
					break;
				}
				case eCA: // Inverted to AC to reuse cached values
				{
					v = (apx * pTri->acx + apy * pTri->acy) / (pTri->acx * pTri->acx + pTri->acy * pTri->acy);
					_pActiveSamples[0] = pTri->Samples[0]->Source.get();
					_pActiveSamples[1] = pTri->Samples[2]->Source.get();
					break;
				}
			}

			UPTR NewDirection;
			if (v < 0.f) NewDirection = vA;
			else if (v > 1.f) NewDirection = vB;
			else break;

			// If we changed contour walking direction, it can be an acute angle. Only one sample is active.
			if (ContourWalkDirection != vC && ContourWalkDirection != NewDirection)
			{
				v = std::clamp(v, 0.f, 1.f);
				break;
			}

			ContourWalkDirection = NewDirection;
			pContourEdge = &_Contour[pContourEdge->Adjacent[NewDirection]];
			pTri = &_Triangles[pContourEdge->TriIndex];
			EdgeIndex = pContourEdge->EdgeIndex;
		}
		while (true);

		u = 1.f - v;
		w = 0.f;

		_pActiveSamples[2] = nullptr;

		//!!!???apply distance-based anim speed scaling like in 1D but for both inputs!?
		// Example from 1D:
		// Scale animation speed for values outside the sample range
		//!!!FIXME: can't clamp this way! should handle outside-a-convex case for this?
		//const float ClampedInput = std::clamp(XInput, _Samples.front().XValue, _Samples.back().XValue);
		//if (ClampedInput && ClampedInput != XInput) dt *= (XInput / ClampedInput);
		break;
	}

	// Get rid of too small weights to avoid sampling animations for them
	constexpr float MINIMAL_WEIGHT = 0.01f;
	bool RenormalizeWeights = false;
	if (_pActiveSamples[0] && u < MINIMAL_WEIGHT)
	{
		u = 0.f;
		_pActiveSamples[0] = nullptr;
		RenormalizeWeights = true;
	}
	if (_pActiveSamples[1] && v < MINIMAL_WEIGHT)
	{
		v = 0.f;
		_pActiveSamples[1] = nullptr;
		RenormalizeWeights = true;
	}
	if (_pActiveSamples[2] && w < MINIMAL_WEIGHT)
	{
		w = 0.f;
		_pActiveSamples[2] = nullptr;
		RenormalizeWeights = true;
	}

	//!!!???TODO: check if samples reference the same animation with the same speed etc!? or blend speeds?
	//!!!CRY: playback scale is blended from active samples by weight! Does support reverse?
	//It is used for dt modification only, so maybe should work OK.

	if (RenormalizeWeights)
	{
		const float Coeff = 1.f / (u + v + w);
		u *= Coeff;
		v *= Coeff;
		w *= Coeff;
	}

	n_assert_dbg(n_fequal(u + v + w, 1.f));

	// Sort samples by weight descending
	if (u < v)
	{
		std::swap(u, v);
		std::swap(_pActiveSamples[0], _pActiveSamples[1]);
	}
	if (u < w)
	{
		std::swap(u, w);
		std::swap(_pActiveSamples[0], _pActiveSamples[2]);
	}
	if (v < w)
	{
		std::swap(v, w);
		std::swap(_pActiveSamples[1], _pActiveSamples[2]);
	}

	n_assert_dbg(_pActiveSamples[0]);

	_Weights[0] = u;
	_Weights[1] = v;
	_Weights[2] = w;

	// Update sample playback cursors

	// TODO: playback rate must be correctly calculated for blend spaces (see CRY and UE4?)
	//???need blend space _NormalizedTime? can always get from master animation
	AdvanceNormalizedTime(dt, GetAnimationLengthScaled(), _NormalizedTime);

	for (size_t i = 0; i < 3; ++i)
	{
		const auto pSample = _pActiveSamples[i];
		if (!pSample) break;
		pSample->Update(Context, dt);
	}

	//{
	//	auto GetSampleIndex = [this](CAnimGraphNode* pNode) -> int
	//	{
	//		for (size_t i = 0; i < _Samples.size(); ++i)
	//			if (_Samples[i].Source.get() == pNode) return static_cast<int>(i);
	//		return -1;
	//	};
	//	::Sys::DbgOut("***CBlendSpace2D: time %lf, ipol [%d]:%lf [%d]:%lf [%d]:%lf\n", _NormalizedTime,
	//		GetSampleIndex(_pActiveSamples[0]), u, GetSampleIndex(_pActiveSamples[1]), v, GetSampleIndex(_pActiveSamples[2]), w);
	//}
}
//---------------------------------------------------------------------

void CBlendSpace2D::EvaluatePose(CPoseBuffer& Output)
{
	if (!_pActiveSamples[0]) return;

	_pActiveSamples[0]->EvaluatePose(Output);

	if (!_pActiveSamples[1]) return;

	Output *= _Weights[0];

	_pActiveSamples[1]->EvaluatePose(_TmpPose);
	Output.Accumulate(_TmpPose, _Weights[1]);

	if (_pActiveSamples[2])
	{
		_pActiveSamples[2]->EvaluatePose(_TmpPose);
		Output.Accumulate(_TmpPose, _Weights[2]);
	}

	Output.NormalizeRotations();
}
//---------------------------------------------------------------------

float CBlendSpace2D::GetAnimationLengthScaled() const
{
	if (!_pActiveSamples[0]) return 0.f;
	if (!_pActiveSamples[1]) return _pActiveSamples[0]->GetAnimationLengthScaled();
	return _pActiveSamples[0]->GetAnimationLengthScaled() * _Weights[0] +
		_pActiveSamples[1]->GetAnimationLengthScaled() * _Weights[1] +
		(_pActiveSamples[2] ? _pActiveSamples[2]->GetAnimationLengthScaled() * _Weights[2] : 0.f);
}
//---------------------------------------------------------------------

// NB: sample value must be different from neighbours at least by SAMPLE_MATCH_TOLERANCE
bool CBlendSpace2D::AddSample(PAnimGraphNode&& Source, float XValue, float YValue)
{
	// TODO: fail if acceleration structure/triangles are not empty, or invalidate them and refresh on next Init/sampling!
	if (!Source) return false;

	for (const auto& Sample : _Samples)
		if (n_fequal(Sample.XValue, XValue, SAMPLE_MATCH_TOLERANCE) && n_fequal(Sample.YValue, YValue, SAMPLE_MATCH_TOLERANCE))
			return false;

	_Samples.push_back({ std::move(Source), XValue, YValue });
	return true;
}
//---------------------------------------------------------------------

}
