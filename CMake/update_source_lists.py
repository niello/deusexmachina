#
# Use this script to update CMake source lists of DEM and its dependencies.
# Call it once when source file set changes and commit results.
#
import os
from utils.utils import update_src_lists

PROJECT_FOLDER = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))


if __name__ == "__main__":
	update_src_lists("DEM_L1_LOW", os.path.join(PROJECT_FOLDER, "CMake", "DEMLow.cmake"), os.path.join(PROJECT_FOLDER, "DEM", "Low", "src"))
	update_src_lists("DEM_L2_GAME", os.path.join(PROJECT_FOLDER, "CMake", "DEMGame.cmake"), os.path.join(PROJECT_FOLDER, "DEM", "Game", "src"))
	update_src_lists("DEM_L3_RPG", os.path.join(PROJECT_FOLDER, "CMake", "DEMRPG.cmake"), os.path.join(PROJECT_FOLDER, "DEM", "RPG", "src"))
	print("Done")
