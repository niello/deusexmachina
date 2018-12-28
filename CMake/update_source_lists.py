import os

PROJECT_FOLDER = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

def update_lists_cpp(var_name, out_file_path, source_folder, root_folder):
	header_list = ""
	source_list = ""
	for dirpath, dirnames, files in os.walk(source_folder):
		for file_name in files:
			ext = os.path.splitext(file_name)[1]
			if ext == '.cpp' or ext == '.cc' or ext == '.c':
				source_list += ("\n\t" + os.path.relpath(os.path.join(dirpath, file_name), root_folder))
			elif ext == '.h':
				header_list += ("\n\t" + os.path.relpath(os.path.join(dirpath, file_name), root_folder))

	out_text = 'set({}_HEADERS{}\n)\n\nset({}_SOURCES{}\n)'.format(var_name, header_list.replace("\\", "/"), var_name, source_list.replace("\\", "/"))

	with open(out_file_path, "w") as file:
		file.write(out_text)


if __name__ == "__main__":
	update_lists_cpp("DEM_L1_LOW", os.path.join(PROJECT_FOLDER, "CMake", "DEMLow.cmake"), os.path.join(PROJECT_FOLDER, "DEM", "Low", "src"), PROJECT_FOLDER)
	update_lists_cpp("DEM_L2_GAME", os.path.join(PROJECT_FOLDER, "CMake", "DEMGame.cmake"), os.path.join(PROJECT_FOLDER, "DEM", "Game", "src"), PROJECT_FOLDER)
	update_lists_cpp("DEM_L3_RPG", os.path.join(PROJECT_FOLDER, "CMake", "DEMRPG.cmake"), os.path.join(PROJECT_FOLDER, "DEM", "RPG", "src"), PROJECT_FOLDER)
	update_lists_cpp("LUA", os.path.join(PROJECT_FOLDER, "Deps", "lua", "src.cmake"), os.path.join(PROJECT_FOLDER, "Deps", "lua", "src"), os.path.join(PROJECT_FOLDER, "Deps", "lua"))
