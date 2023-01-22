#pragma once
#include <Core/Object.h>

// A reusable Lua script without per-user-instance state

namespace DEM::Scripting
{

class CScriptAsset : public ::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Game::CEntityTemplate, ::Core::CObject);

protected:

	std::string _SourceBuffer;

	// TODO: if sol state will be per application and not per session, can store sol::table here instead of or along with source!

public:

	CScriptAsset(std::string&& SourceBuffer) : _SourceBuffer(std::move(SourceBuffer)) {}

	const std::string& GetSourceBuffer() const { return _SourceBuffer; }
};

typedef Ptr<CScriptAsset> PScriptAsset;

}
