#
# Use this script to update CMake source lists of DEM and its dependencies.
# Call it once when source file set changes, and commit results.
#
import os
from utils.utils import update_src_lists

PROJECT_FOLDER = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))


if __name__ == "__main__":
	# Engine
	update_src_lists("DEM_L1_LOW", os.path.join(PROJECT_FOLDER, "CMake", "DEMLow.cmake"), os.path.join(PROJECT_FOLDER, "DEM", "Low", "src"))
	update_src_lists("DEM_L2_GAME", os.path.join(PROJECT_FOLDER, "CMake", "DEMGame.cmake"), os.path.join(PROJECT_FOLDER, "DEM", "Game", "src"))
	update_src_lists("DEM_L3_RPG", os.path.join(PROJECT_FOLDER, "CMake", "DEMRPG.cmake"), os.path.join(PROJECT_FOLDER, "DEM", "RPG", "src"))
	
	# Tools
	update_src_lists("DEM_TOOLS_COMMON", os.path.join(PROJECT_FOLDER, "Tools", "Common", "src.cmake"))
	update_src_lists("DEM_SCENE_COMMON", os.path.join(PROJECT_FOLDER, "Tools", "SceneCommon", "src.cmake"))
	update_src_lists("DEM_SHADER_COMPILER", os.path.join(PROJECT_FOLDER, "Tools", "ShaderCompiler", "src.cmake"), os.path.join(PROJECT_FOLDER, "Tools", "ShaderCompiler", "src"))
	update_src_lists("DEM_CF_HLSL", os.path.join(PROJECT_FOLDER, "Tools", "ContentForge", "cf-hlsl", "src.cmake"))
	update_src_lists("DEM_CF_EFFECT", os.path.join(PROJECT_FOLDER, "Tools", "ContentForge", "cf-effect", "src.cmake"))
	update_src_lists("DEM_CF_RPATH", os.path.join(PROJECT_FOLDER, "Tools", "ContentForge", "cf-rpath", "src.cmake"))
	update_src_lists("DEM_CF_MTL", os.path.join(PROJECT_FOLDER, "Tools", "ContentForge", "cf-material", "src.cmake"))
	update_src_lists("DEM_CF_FBX", os.path.join(PROJECT_FOLDER, "Tools", "ContentForge", "cf-fbx", "src.cmake"))
	update_src_lists("DEM_CF_GLTF", os.path.join(PROJECT_FOLDER, "Tools", "ContentForge", "cf-gltf", "src.cmake"))
	update_src_lists("DEM_CF_L3DT", os.path.join(PROJECT_FOLDER, "Tools", "ContentForge", "cf-l3dt", "src.cmake"))
	update_src_lists("DEM_CF_SKY", os.path.join(PROJECT_FOLDER, "Tools", "ContentForge", "cf-skybox", "src.cmake"))
	update_src_lists("DEM_CF_NAV", os.path.join(PROJECT_FOLDER, "Tools", "ContentForge", "cf-navmesh", "src.cmake"))

	print("Done updating engine source lists")
