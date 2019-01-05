rem Creates 'deusexmachina' folder, clones repo, configures, builds and opens engine
rem Now MSVC 2017 is hardcoded, mb can use default generator?

git.exe clone --progress --recursive -v "https://github.com/niello/deusexmachina.git"

cmake -G "Visual Studio 15 2017" -DCMAKE_CONFIGURATION_TYPES:STRING=Debug;Release -S deusexmachina -Bdeusexmachina/Build/Win
cmake --build deusexmachina/Build/Win --target ALL_BUILD --config Debug
cmake --build deusexmachina/Build/Win --target ALL_BUILD --config Release

start deusexmachina/Build/Win/deusexmachina.sln