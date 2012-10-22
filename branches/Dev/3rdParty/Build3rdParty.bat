%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Debug;Platform="Win32" Xiph\Build\VS2008\Xiph.sln

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Release;Platform="Win32" Xiph\Build\VS2008\Xiph.sln

rem pause

pushd .
cd ODE\build
premake4.exe vs2008
popd

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=DebugSingleLib;Platform="Win32" ODE\build\vs2008\ode.sln

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=ReleaseSingleLib;Platform="Win32" ODE\build\vs2008\ode.sln

rem pause

pushd .
cd CEGUI\projects\premake
premake.exe --file cegui_DEM.lua --target vs2008
popd

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Debug_Static;Platform="Win32" CEGUI\projects\premake\CEGUI.sln

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Release_Static;Platform="Win32" CEGUI\projects\premake\CEGUI.sln

rem pause

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Debug;Platform="Win32" Lua\Build\VS2008\lua51.sln

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Release;Platform="Win32" Lua\Build\VS2008\lua51.sln

rem pause

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Debug;Platform="Win32" Recast\RecastLibs\Build\VS2008\RecastLibs.sln

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Release;Platform="Win32" Recast\RecastLibs\Build\VS2008\RecastLibs.sln

rem pause

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Debug;Platform="Win32" SQLite\Build\VS2008\sqlite.sln

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Release;Platform="Win32" SQLite\Build\VS2008\sqlite.sln

rem pause

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Debug;Platform="Win32" TinyXML2\Build\VS2008\TinyXML2.sln

%windir%\Microsoft.NET\Framework\v3.5\msbuild /property:Configuration=Release;Platform="Win32" TinyXML2\Build\VS2008\TinyXML2.sln

rem pause
