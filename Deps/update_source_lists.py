#
# Use this script to update CMake source lists of DEM and its dependencies.
# Call it once when source file set changes and commit results.
#
import os

PROJECT_FOLDER = os.path.dirname(os.path.realpath(__file__))

def update_lists_cpp(var_name, out_file_path, source_folder, append=False):
	root_folder = os.path.commonprefix([os.path.dirname(out_file_path), source_folder])
	header_list = ""
	source_list = ""
	for dirpath, dirnames, files in os.walk(source_folder):
		for file_name in files:
			ext = os.path.splitext(file_name)[1]
			if ext == '.cpp' or ext == '.cc' or ext == '.c' or ext == '.cxx':
				source_list += ("\n\t" + os.path.relpath(os.path.join(dirpath, file_name), root_folder))
			elif ext == '.h' or ext == '.inc' or ext == '.hpp':
				header_list += ("\n\t" + os.path.relpath(os.path.join(dirpath, file_name), root_folder))

	out_text = ""
	if header_list:
		out_text += 'set({}_HEADERS{}\n)\n\n'.format(var_name, header_list.replace("\\", "/"))
	if source_list:
		out_text += 'set({}_SOURCES{}\n)\n\n'.format(var_name, source_list.replace("\\", "/"))

	with open(out_file_path, "a" if append else "w") as file:
		file.write(out_text)


if __name__ == "__main__":
	update_lists_cpp("LUA", os.path.join(PROJECT_FOLDER, "lua", "src.cmake"), os.path.join(PROJECT_FOLDER, "lua", "src"))
