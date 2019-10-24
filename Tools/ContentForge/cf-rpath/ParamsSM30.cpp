#include <CFRenderPathFwd.h>
#include <Render/ShaderMetaCommon.h>
#include <Render/SM30ShaderMeta.h>
#include <Logging.h>
#include <ParamsCommon.h>
#include <Utils.h>
#include <sstream>

bool MergeParams(CSM30EffectMeta& Src, CSM30EffectMeta& Dest, CThreadSafeLog* pLog)
{
	for (auto& Const : Src.Consts)
	{
		auto& Param = Const.second.second;

		auto ItPrev = Dest.Consts.find(Const.first);
		if (ItPrev == Dest.Consts.end())
		{
			auto& Regs =
				(Param.RegisterSet == RS_Float4) ? Dest.UsedFloat4 :
				(Param.RegisterSet == RS_Int4) ? Dest.UsedInt4 :
				Dest.UsedBool;

			const auto RegisterEnd = Param.RegisterStart + Param.ElementRegisterCount * Param.ElementCount;
			for (uint32_t r = Param.RegisterStart; r < RegisterEnd; ++r)
				Regs.insert(r);

			CopyBufferMetadata(Param.BufferIndex, Src.Buffers, Dest.Buffers);
			CopyStructMetadata(Param.StructIndex, Src.Structs, Dest.Structs);

			Dest.Consts.insert(std::move(Const));
		}
		else
		{
			// The same param found, check compatibility
			const auto& ExistingMeta = ItPrev->second.second;
			if (ExistingMeta != Param)
			{
				if (pLog) pLog->LogError("Constant '" + Param.Name + "' is not compatible across all effects");
				return false;
			}

			// Merge constant buffers instead of comparison, because unused parameters in some shaders
			// may render buffers inequal, but they are still compatible. Overlapping not checked because
			// it will be when adding constants. Probably it is insufficient but acceptable for legacy D3D9.
			MergeConstantBuffers(Src.Buffers[Param.BufferIndex], Dest.Buffers[ExistingMeta.BufferIndex]);

			// Extend shader mask
			ItPrev->second.first |= Const.second.first;
		}
	}

	for (auto& Rsrc : Src.Resources)
	{
		auto& Param = Rsrc.second.second;

		auto ItPrev = Dest.Resources.find(Rsrc.first);
		if (ItPrev == Dest.Resources.end())
		{
			Dest.UsedResources.insert(Param.Register);
			Dest.Resources.insert(std::move(Rsrc));
		}
		else
		{
			// The same param found, check compatibility
			const auto& ExistingMeta = ItPrev->second.second;
			if (ExistingMeta != Param)
			{
				if (pLog) pLog->LogError("Resource '" + Param.Name + "' is not compatible across all effects");
				return false;
			}

			// Extend shader mask
			ItPrev->second.first |= Rsrc.second.first;
		}
	}

	for (auto& Sampler : Src.Samplers)
	{
		auto& Param = Sampler.second.second;

		auto ItPrev = Dest.Samplers.find(Sampler.first);
		if (ItPrev == Dest.Samplers.end())
		{
			for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
				Dest.UsedSamplers.insert(r);

			Dest.Samplers.insert(std::move(Sampler));
		}
		else
		{
			// The same param found, check compatibility
			const auto& ExistingMeta = ItPrev->second.second;
			if (ExistingMeta != Param)
			{
				if (pLog) pLog->LogError("Sampler '" + Param.Name + "' is not compatible across all tech shaders");
				return false;
			}

			// Extend shader mask
			ItPrev->second.first |= Sampler.second.first;
		}
	}

	return true;
}
//---------------------------------------------------------------------

bool VerifyParams(const CSM30EffectMeta& Meta, const CSM30EffectMeta& Against, CThreadSafeLog* pLog)
{
	for (const auto& Const : Against.Consts)
	{
		const auto& Param = Const.second.second;
		if (!CheckConstRegisterOverlapping(Param, Meta))
		{
			if (pLog) pLog->LogError(Against.PrintableName + " constant '" + Param.Name + "' uses a register used by global params");
			return false;
		}
	}

	for (const auto& Rsrc : Against.Resources)
	{
		const auto& Param = Rsrc.second.second;
		if (!CheckRegisterOverlapping(Param.Register, 1, Meta.UsedResources))
		{
			if (pLog) pLog->LogError(Against.PrintableName + " resource '" + Param.Name + "' uses a register used by global params");
			return false;
		}
	}

	for (const auto& Sampler : Against.Samplers)
	{
		const auto& Param = Sampler.second.second;
		if (!CheckRegisterOverlapping(Param.RegisterStart, Param.RegisterCount, Meta.UsedSamplers))
		{
			if (pLog) pLog->LogError(Against.PrintableName + " sampler '" + Param.Name + "' uses a register used by global params");
			return false;
		}
	}

	return true;
}
//---------------------------------------------------------------------

bool BuildGlobalsTableForDX9C(CGlobalTable& Task, CThreadSafeLog* pLog)
{
	CSM30EffectMeta RPGlobalMeta;
	RPGlobalMeta.PrintableName = "RP global";

	// Load metadata from all effects, merge and verify compatibility

	std::vector<CSM30EffectMeta> MetaToCheck;

	for (const auto& Src : Task.Sources)
	{
		auto& InStream = *Src.Stream;

		// Skip 32-bit metadata size
		InStream.seekg(Src.Offset + sizeof(uint32_t), std::ios_base::beg);

		CSM30EffectMeta EffectGlobalMeta;
		EffectGlobalMeta.PrintableName = "Effect global";
		InStream >> EffectGlobalMeta;

		if (!MergeParams(EffectGlobalMeta, RPGlobalMeta, pLog)) return false;

		if (!CollectNonGlobalMetadataFromEffect(InStream, MetaToCheck, pLog)) return false;
	}

	for (const auto& Meta : MetaToCheck)
		if (!VerifyParams(RPGlobalMeta, Meta, pLog)) return false;

	std::ostringstream OutStream(std::ios_base::binary);
	OutStream << RPGlobalMeta;
	Task.Result = OutStream.str();

	return true;
}
//---------------------------------------------------------------------
