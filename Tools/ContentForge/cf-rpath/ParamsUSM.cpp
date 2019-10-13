#include <CFRenderPathFwd.h>
#include <Render/ShaderMetaCommon.h>
#include <Render/USMShaderMeta.h>
#include <Logging.h>
#include <ParamsCommon.h>
#include <Utils.h>
#include <sstream>

bool MergeParams(CUSMEffectMeta& Src, CUSMEffectMeta& Dest, CThreadSafeLog* pLog)
{
	for (auto& Const : Src.Consts)
	{
		auto& Param = Const.second.second;

		auto ItPrev = Dest.Consts.find(Const.first);
		if (ItPrev == Dest.Consts.end())
		{
			Dest.UsedConstantBuffers.insert(Src.Buffers[Param.BufferIndex].Register);

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

			// Compare containing constant buffers
			if (Dest.Buffers[ExistingMeta.BufferIndex] != Src.Buffers[Param.BufferIndex])
			{
				if (pLog) pLog->LogError("Constant '" + Param.Name + "' containing buffer is not compatible across all effects");
				return false;
			}

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
			for (uint32_t r = Param.RegisterStart; r < Param.RegisterStart + Param.RegisterCount; ++r)
				Dest.UsedResources.insert(r);

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

bool VerifyParams(const CUSMEffectMeta& Meta, const CUSMEffectMeta& Against, CThreadSafeLog* pLog)
{
	for (const auto& Const : Against.Consts)
	{
		const auto& Param = Const.second.second;
		const uint32_t BufferRegister = Against.Buffers[Param.BufferIndex].Register;
		if (!CheckRegisterOverlapping(BufferRegister, 1, Meta.UsedConstantBuffers))
		{
			if (pLog) pLog->LogError(Against.PrintableName + " buffer containing '" + Param.Name + "' uses a register used by global params");
			return false;
		}
	}

	for (const auto& Rsrc : Against.Resources)
	{
		const auto& Param = Rsrc.second.second;
		if (!CheckRegisterOverlapping(Param.RegisterStart, Param.RegisterCount, Meta.UsedResources))
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

bool BuildGlobalsTableForDXBC(CGlobalTable& Task, CThreadSafeLog* pLog)
{
	CUSMEffectMeta RPGlobalMeta;
	RPGlobalMeta.PrintableName = "RP Global";

	// Load metadata from all effects, merge and verify compatibility

	std::vector<CUSMEffectMeta> MetaToCheck;

	for (const auto& Src : Task.Sources)
	{
		auto& InStream = *Src.Stream;

		InStream.seekg(Src.Offset, std::ios_base::beg);

		CUSMEffectMeta Buffer;
		Buffer.PrintableName = "Material";
		InStream >> Buffer;

		if (!MergeParams(Buffer, RPGlobalMeta, pLog)) return false;

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
