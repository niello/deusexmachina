extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

#include <Data/Streams/FileStream.h>
#include <Data/Streams/MemStream.h>
#include <Data/Buffer.h>
#include <Data/DataServer.h>

lua_State* l = NULL;

bool LuaInit()
{
	n_assert(!l);
	l = luaL_newstate();
	//luaL_openlibs(l);
	return !!l;
}

void LuaRelease()
{
	if (l)
	{
		lua_close(l);
		l = NULL;
	}
}

// Saves Lua data to stream
static int SaveLua(lua_State* L, const void* p, size_t sz, void* ud)
{
	n_assert(ud);
	Data::CStream* pOut = (Data::CStream*)ud;
	return (pOut->Write(p, sz) == sz) ? 0 : 1;
}

bool LuaCompile(char* pData, uint Size, LPCSTR Name, LPCSTR pFileOut)
{
	if (!l) LuaInit();

	if (luaL_loadbuffer(l, pData, Size, Name) != 0)
	{
		n_printf("Error parsing Lua file '%s': %s\n", Name, lua_tostring(l, -1));
		lua_pop(l, 1); // Error msg
		FAIL;
	}

	int Result = 1;

	Data::CFileStream Out;
	if (Out.Open(pFileOut, Data::SAM_WRITE, Data::SAP_SEQUENTIAL))
	{
		Result = lua_dump(l, SaveLua, (Data::CStream*)&Out);
		Out.Close();
	}

	return !Result;
}

bool LuaCompileClass(Data::CParams& LoadedHRD, LPCSTR Name, LPCSTR pFileOut)
{
	nString Code = LoadedHRD.Get<nString>(CStrID("Code"), NULL);

	if (Code.IsValid())
	{
		if (luaL_loadbuffer(l, Code.Get(), Code.Length(), Name) != 0)
		{
			n_printf("Error parsing Lua file '%s': %s\n", Name, lua_tostring(l, -1));
			lua_pop(l, 1); // Error msg
			FAIL;
		}

		int Result = 1;

		Data::CMemStream Out;
		if (Out.Open(Data::SAM_WRITE, Data::SAP_SEQUENTIAL))
		{
			Result = lua_dump(l, SaveLua, (Data::CStream*)&Out);
			Data::CBuffer Buffer(Out.GetPtr(), Out.GetSize());
			Out.Close();
			if (Result != 0) FAIL;

			LoadedHRD.Set(CStrID("Code"), Buffer);
		}
		else FAIL;
	}

	DataSrv->SavePRM(pFileOut, &LoadedHRD);

	OK;
}