All dependencies with Visual Studio solutions provided are built as static
libraries without C++ exceptions, without C++ RTTI and with static C runtime.
You may need to build other 3rdParty libs with the same settings.
We try to provide up-to-date solution & project files for each our dependency.

Precompiled binaries for Win32 may be found in Downloads section, at
http://code.google.com/p/deusexmachina/downloads/list

Precompiled binary archives are always packed so that you should place it into
this "3rdParty" folder and extract there. Typically binaries reside in
"3rdParty/<ProjectName>/Lib" for static libraries and in
"3rdParty/<ProjectName>/Bin" for dynamic libraries and executables.

"3rdParty/<ProjectName>/Obj" is reserved for intermediate compiler/linker output.
All these folders are added to svn:ignore.
