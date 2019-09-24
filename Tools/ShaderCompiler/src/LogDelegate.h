#pragma once

namespace DEMShaderCompiler
{

class ILogDelegate
{
public:

	virtual void Log(const char* pMessage) = 0;
};

}
