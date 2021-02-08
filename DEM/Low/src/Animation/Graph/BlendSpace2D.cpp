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

void CBlendSpace2D::Init(CAnimationControllerInitContext& Context)
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

		// Ensure starting triangle is first
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

		for (auto& Sample : _Samples)
			Sample.Source->Init(Context);

		// Call after initializing sources, letting them to contribute to SkeletonInfo
		if (Context.SkeletonInfo)
			_Blender.Initialize(3, Context.SkeletonInfo->GetNodeCount());
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

//!!!DUPLICATED CODE! See 1D!
void CBlendSpace2D::UpdateSingleSample(CAnimGraphNode& Node, CAnimationController& Controller, float dt, CSyncContext* pSyncContext)
{
	const ESyncMethod SyncMethod = pSyncContext ? pSyncContext->Method : ESyncMethod::None;
	switch (SyncMethod)
	{
		case ESyncMethod::None:
		{
			AdvanceNormalizedTime(dt, Node.GetAnimationLengthScaled(), _NormalizedTime);
			break;
		}
		case ESyncMethod::NormalizedTime:
		case ESyncMethod::PhaseMatching: //???or get _NormalizedTime from _pActiveSamples[0] current phase?
		{
			_NormalizedTime = pSyncContext->NormalizedTime;
			break;
		}
		default: NOT_IMPLEMENTED; break;
	}

	_pActiveSamples[0] = &Node;
	_pActiveSamples[1] = nullptr;
	_pActiveSamples[2] = nullptr;
	_pActiveSamples[0]->Update(Controller, dt, pSyncContext);
}
//---------------------------------------------------------------------

void CBlendSpace2D::Update(CAnimationController& Controller, float dt, CSyncContext* pSyncContext)
{
	if (_Samples.empty()) return;

	if (_Samples.size() == 1)
	{
		//???!!!after fixind dt due to scaling based on distance from input to sample?! See 1D.
		UpdateSingleSample(*_Samples[0].Source, Controller, dt, pSyncContext);
		return;
	}

	const float XInput = Controller.GetFloat(_XParamIndex);
	const float YInput = Controller.GetFloat(_YParamIndex);

	//???use cache if input parameter didn't change?

	//???TODO: optionally filter input to make transitions smoother?
	// then dt *= (BeforeFilter / AfterFilter);

	// Find triangle or segment of active samples
	// FIXME: support degenerate cases - point or collinear!
	float u, v, w;
	size_t CurrTriIndex = 0;
	while (true)
	{
		const auto& Tri = _Triangles[CurrTriIndex];
		const float apx = XInput - Tri.ax;
		const float apy = YInput - Tri.ay;
		v = (apx * Tri.acy - Tri.acx * apy) * Tri.InvDenominator;
		w = (Tri.abx * apy - apx * Tri.aby) * Tri.InvDenominator;
		u = 1.f - v - w;
		const auto NegativeMask = (static_cast<int>(w < 0.f) << 2) | (static_cast<int>(v < 0.f) << 1) | static_cast<int>(u < 0.f);

		// Zero mask means that all weights are positive and we are inside a triangle
		if (!NegativeMask)
		{
			_pActiveSamples[0] = Tri.Samples[0]->Source.get();
			_pActiveSamples[1] = Tri.Samples[1]->Source.get();
			_pActiveSamples[2] = Tri.Samples[2]->Source.get();
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

		CurrTriIndex = Tri.Adjacent[EdgeIndex];
		if (CurrTriIndex == INVALID_INDEX)
		{
			// We moved outside the convex poly, project input onto the border segment
			switch (EdgeIndex)
			{
				case eAB:
				{
					v = (apx * Tri.abx + apy * Tri.aby) / (Tri.abx * Tri.abx + Tri.aby * Tri.aby);
					_pActiveSamples[0] = Tri.Samples[0]->Source.get();
					_pActiveSamples[1] = Tri.Samples[1]->Source.get();
					break;
				}
				case eBC:
				{
					// We do not cache this because it adds 2 floats to every triangle
					// but is useful only for triangles with BC being a border edge
					const float bcx = Tri.Samples[2]->XValue - Tri.Samples[1]->XValue;
					const float bcy = Tri.Samples[2]->YValue - Tri.Samples[1]->YValue;
					const float bpx = XInput - Tri.Samples[1]->XValue;
					const float bpy = YInput - Tri.Samples[1]->YValue;

					v = (bpx * bcx + bpy * bcy) / (bcx * bcx + bcy * bcy);
					_pActiveSamples[0] = Tri.Samples[1]->Source.get();
					_pActiveSamples[1] = Tri.Samples[2]->Source.get();
					break;
				}
				case eCA: // Inverted to AC to reuse cached values
				{
					v = (apx * Tri.acx + apy * Tri.acy) / (Tri.acx * Tri.acx + Tri.acy * Tri.acy);
					_pActiveSamples[0] = Tri.Samples[0]->Source.get();
					_pActiveSamples[1] = Tri.Samples[2]->Source.get();
					break;
				}
			}

			v = std::clamp(v, 0.f, 1.f);
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

	//!!!must propagate locomotion phase!
	if (!_pActiveSamples[1])
	{
		UpdateSingleSample(*_pActiveSamples[0], Controller, dt, pSyncContext);
		return;
	}

	_Blender.SetWeight(0, u);
	_Blender.SetWeight(1, v);
	_Blender.SetWeight(2, w);

	// Update sample playback cursors

	/*
	{
		CSyncContext LocalSyncContext{ ESyncMethod::None };

		for (size_t i = 0; i < 3; ++i)
		{
			const auto pSample = _pActiveSamples[i];
			if (!pSample) break;

			//!!!if sample was inactive, activate it! OnActivate() will happen, e.g. to reset anim to start time.

			if (pSample->HasLocomotion())
			{
				//???get locomotion phase when sampling the clip and propagate up by writing it to the sync context?
				//only if clip is locomotion and no phase is passed down?
				if (LocalSyncContext.Method != ESyncMethod::PhaseMatching)
				{
					//!!!instead of -99999.f => Controller.GetLocomotionPhaseFromPose()! Do once and cache (in sync context?)
					const float Phase = (i > 0) ? _pActiveSamples[i - 1]->GetLocomotionPhase() : -99999.f;
					if (Phase >= 0.f)
					{
						LocalSyncContext.Method = ESyncMethod::PhaseMatching;
						LocalSyncContext.LocomotionPhase = Phase;
					}
				}

				//!!!NB: if phase is set externally, must apply it! Make sure it happens!
			}

			//if phase is set, request phase-synced update
			//else if need time-syncing, request time-synced update
			//else play sample with dt and without syncing
			pSample->Update(Controller, dt, &LocalSyncContext);
		}
	}
	*/

	// - need child sample (de)activation, may be very useful not only for resetting
	// - phase syncing must be correctly implemented, including external poses
	// - sync normalized time to be synced with phase? Sync locomotion cycle with non-locomotion action. Need?
	// - playback rate must be correctly calculated for blend spaces (see CRY and UE4?)

	// Next:
	// - selector (CStrID based?) with blend time for switching to actions like "open door". Finish vs cancel anim?
	// - pose buffer(s)
	// - pose modifiers = skeletal controls, object space
	// - inertialization
	// - IK

	const ESyncMethod SyncMethod = pSyncContext ? pSyncContext->Method : ESyncMethod::None;
	switch (SyncMethod)
	{
		case ESyncMethod::None:
		{
			//???need blend space _NormalizedTime? can always get from master animation
			AdvanceNormalizedTime(dt, GetAnimationLengthScaled(), _NormalizedTime);

			//CSyncContext LocalSyncContext{ ESyncMethod::NormalizedTime, _NormalizedTime };
			CSyncContext LocalSyncContext{ ESyncMethod::None };

			for (size_t i = 0; i < 3; ++i)
			{
				const auto pSample = _pActiveSamples[i];
				if (!pSample) break;

				//???get locomotion phase when sampling the clip and propagate up by writing it to the sync context?
				//only if clip is locomotion and no phase is passed down?
				if (LocalSyncContext.Method != ESyncMethod::PhaseMatching && pSample->HasLocomotion())
				{
					//!!!else (-99999.f) get from initial pose at the start of the frame!
					const float Phase = (i > 0) ? _pActiveSamples[i - 1]->GetLocomotionPhase() : -99999.f;
					if (Phase >= 0.f)
					{
						LocalSyncContext.Method = ESyncMethod::PhaseMatching;
						LocalSyncContext.LocomotionPhase = Phase;
					}
				}

				pSample->Update(Controller, dt, &LocalSyncContext);
			}

			break;
		}
		case ESyncMethod::NormalizedTime:
		case ESyncMethod::PhaseMatching:
		{
			_NormalizedTime = pSyncContext->NormalizedTime;
			//???if phase matching, get _NormalizedTime from curr _pActiveSamples[0] phase?

			_pActiveSamples[0]->Update(Controller, dt, pSyncContext);
			_pActiveSamples[1]->Update(Controller, dt, pSyncContext);
			if (_pActiveSamples[2]) _pActiveSamples[2]->Update(Controller, dt, pSyncContext);

			break;
		}
		default: NOT_IMPLEMENTED; break;
	}

	{
		auto GetSampleIndex = [this](CAnimGraphNode* pNode) -> int
		{
			for (size_t i = 0; i < _Samples.size(); ++i)
				if (_Samples[i].Source.get() == pNode) return static_cast<int>(i);
			return -1;
		};
		::Sys::DbgOut("***CBlendSpace2D: time %lf, ipol [%d]:%lf [%d]:%lf [%d]:%lf\n", _NormalizedTime,
			GetSampleIndex(_pActiveSamples[0]), u, GetSampleIndex(_pActiveSamples[1]), v, GetSampleIndex(_pActiveSamples[2]), w);
	}
}
//---------------------------------------------------------------------

void CBlendSpace2D::EvaluatePose(IPoseOutput& Output)
{
	if (_pActiveSamples[1])
	{
		//???TODO: try inplace blending in Output instead of preallocated Blender? helps to save
		// memory and may be faster! We don't use priority here anyway, and weights always sum to 1.f.
		//Can use special wrapper output CPoseScaleBiasOutput / CPoseWeightedOutput to apply weights on the fly!
		//!!!can even have single and per-bone weigh variations!
		//???cache locality may suffer if blending in place? scene nodes are scattered around the heap.
		_pActiveSamples[0]->EvaluatePose(*_Blender.GetInput(0));
		_pActiveSamples[1]->EvaluatePose(*_Blender.GetInput(1));
		if (_pActiveSamples[2]) _pActiveSamples[2]->EvaluatePose(*_Blender.GetInput(2));

		_Blender.EvaluatePose(Output);
	}
	else if (_pActiveSamples[0]) _pActiveSamples[0]->EvaluatePose(Output);
}
//---------------------------------------------------------------------

float CBlendSpace2D::GetAnimationLengthScaled() const
{
	if (!_pActiveSamples[0]) return 0.f;
	if (!_pActiveSamples[1]) return _pActiveSamples[0]->GetAnimationLengthScaled();
	return _pActiveSamples[0]->GetAnimationLengthScaled() * _Blender.GetWeight(0) +
		_pActiveSamples[1]->GetAnimationLengthScaled() * _Blender.GetWeight(1) +
		(_pActiveSamples[2] ? _pActiveSamples[2]->GetAnimationLengthScaled() * _Blender.GetWeight(2) : 0.f);
}
//---------------------------------------------------------------------

float CBlendSpace2D::GetLocomotionPhase() const
{
	// Always from the most weighted animation, others must be synchronized
	//???what if locomotion has less weight than idle?
	return _pActiveSamples[0] ? _pActiveSamples[0]->GetLocomotionPhase() : std::numeric_limits<float>().lowest();
}
//---------------------------------------------------------------------

bool CBlendSpace2D::HasLocomotion() const
{
	return (_pActiveSamples[0] && _pActiveSamples[0]->HasLocomotion()) ||
		(_pActiveSamples[1] && _pActiveSamples[1]->HasLocomotion()) ||
		(_pActiveSamples[2] && _pActiveSamples[2]->HasLocomotion());
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
