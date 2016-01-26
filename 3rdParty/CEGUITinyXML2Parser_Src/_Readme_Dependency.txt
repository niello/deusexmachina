1. Generate static CEGUI with CMake to a '<CEGUI_ROOT>/build/VS2013', make sure PCRE editbox validation is enabled, as things like spinners require it
2. Copy 'build' and 'cegui' folders to '<CEGUI_ROOT>', merge with existing ones
3. Add '<CEGUI_ROOT>/build/VS2013/cegui/src/XMLParserModules/CEGUITinyXML2Parser_Static.vcxproj' to the solution
4. Set output directory for all static projects as '$(SolutionDir)..\..\lib\'
