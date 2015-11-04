1. Generate static CEGUI with CMake to a '<CEGUI_ROOT>/build/VS2013'
2. Set output directory for all static projects as '$(SolutionDir)..\..\lib\'
3. Place a project file 'CEGUITinyXML2Parser_Static.vcxproj' to a 'cegui\src\XMLParserModules' subfolder of that folder
4. Place source and include files to appropriate directories (exactly the same as TinyXML parser, but create 'TinyXML2' folders)
