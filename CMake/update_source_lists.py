import os

PROJECT_FOLDER = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

def update_lists_cpp(var_name, out_file_name, source_folder):
	header_list = ""
	source_list = ""
	for dirpath, dirnames, files in os.walk(source_folder):
		for file_name in files:
			ext = os.path.splitext(file_name)[1]
			if ext == '.cpp' or ext == '.cc' or ext == '.c':
				source_list += ("\n\t" + os.path.relpath(os.path.join(dirpath, file_name), PROJECT_FOLDER))
			elif ext == '.h':
				header_list += ("\n\t" + os.path.relpath(os.path.join(dirpath, file_name), PROJECT_FOLDER))

	out_text = 'set({}_HEADERS{}\n)\n\nset({}_SOURCES{}\n)'.format(var_name, header_list.replace("\\", "/"), var_name, source_list.replace("\\", "/"))

	out_file_path = os.path.join(PROJECT_FOLDER, "CMake", out_file_name)
	with open(out_file_path, "w") as file:
		file.write(out_text)


if __name__ == "__main__":
	update_lists_cpp("DEM_L1_LOW", "DEMLow.cmake", os.path.join(PROJECT_FOLDER, "DEM", "Low", "src"))
	update_lists_cpp("DEM_L2_GAME", "DEMGame.cmake", os.path.join(PROJECT_FOLDER, "DEM", "Game", "src"))
	update_lists_cpp("DEM_L3_RPG", "DEMRPG.cmake", os.path.join(PROJECT_FOLDER, "DEM", "RPG", "src"))
