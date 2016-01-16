extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

#include <IO/Streams/FileStream.h>
#include <IO/Streams/MemStream.h>
#include <IO/BinaryWriter.h>
#include <Data/Buffer.h>
#include <Data/Params.h>
#include <ConsoleApp.h>

lua_State* l = NULL;

bool LuaInit()
{
	n_assert(!l);
	l = luaL_newstate();
	//luaL_openlibs(l);
	return !!l;
}
//---------------------------------------------------------------------

void LuaRelease()
{
	if (l)
	{
		lua_close(l);
		l = NULL;
	}
}
//---------------------------------------------------------------------

// Saves Lua data to stream
static int SaveLua(lua_State* L, const void* p, size_t sz, void* ud)
{
	n_assert(ud);
	IO::CStream* pOut = (IO::CStream*)ud;
	return (pOut->Write(p, sz) == sz) ? 0 : 1;
}
//---------------------------------------------------------------------

bool LuaCompile(char* pData, U32 Size, const char* Name, const char* pFileOut)
{
	if (!l) LuaInit();

	if (luaL_loadbuffer(l, pData, Size, Name) != 0)
	{
		n_msg(VL_ERROR, "Error parsing Lua file '%s': %s\n", Name, lua_tostring(l, -1));
		lua_pop(l, 1); // Error msg
		FAIL;
	}

	int Result = 1;

	IO::CFileStream Out(pFileOut);
	if (Out.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
	{
		Result = lua_dump(l, SaveLua, (IO::CStream*)&Out);
		Out.Close();
	}

	return !Result;
}
//---------------------------------------------------------------------

bool LuaCompileClass(Data::CParams& LoadedHRD, const char* Name, const char* pFileOut)
{
	CString Code = LoadedHRD.Get<CString>(CStrID("Code"), CString::Empty);

	if (Code.IsValid())
	{
		if (luaL_loadbuffer(l, Code.CStr(), Code.GetLength(), Name) != 0)
		{
			n_msg(VL_ERROR, "Error parsing Lua file '%s': %s\n", Name, lua_tostring(l, -1));
			lua_pop(l, 1); // Error msg
			FAIL;
		}

		int Result = 1;

		IO::CMemStream Out;
		if (Out.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
		{
			Result = lua_dump(l, SaveLua, (IO::CStream*)&Out);
			Data::CBuffer Buffer(Out.GetPtr(), (UPTR)Out.GetSize());
			Out.Close();
			if (Result != 0) FAIL;

			LoadedHRD.Set(CStrID("Code"), Buffer);
		}
		else FAIL;
	}

	IO::CFileStream File(pFileOut);
	if (!File.Open(IO::SAM_WRITE)) FAIL;
	IO::CBinaryWriter Writer(File);
	return Writer.WriteParams(LoadedHRD);

	OK;
}
//---------------------------------------------------------------------
