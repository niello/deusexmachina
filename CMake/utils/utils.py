import os


SOURCE_EXT_LIST = ['.cpp', '.cc', '.c', '.cxx']
HEADER_EXT_LIST = ['.h', '.inc', '.hpp', '.inl']


def update_src_lists(var_name, out_file_path, source_folder=None, append=False,
                     root_folder=None, source_ext_list=SOURCE_EXT_LIST,
                     header_ext_list=HEADER_EXT_LIST):
    if source_folder is None:
        source_folder = os.path.dirname(out_file_path)
    if root_folder is None:
        root_folder = os.path.commonprefix(
            [os.path.dirname(out_file_path), source_folder])
    header_list = ""
    source_list = ""
    for dirpath, dirnames, files in os.walk(source_folder):
        for file_name in files:
            ext = os.path.splitext(file_name)[1]
            if ext in source_ext_list:
                source_list += ("\n\t" + os.path.relpath(
                    os.path.join(dirpath, file_name), root_folder))
            elif ext in header_ext_list:
                header_list += ("\n\t" + os.path.relpath(
                    os.path.join(dirpath, file_name), root_folder))

    out_text = ""
    if header_list:
        out_text += 'set({}_HEADERS{}\n)\n\n'.format(
            var_name, header_list.replace("\\", "/"))
    if source_list:
        out_text += 'set({}_SOURCES{}\n)\n\n'.format(
            var_name, source_list.replace("\\", "/"))

    with open(out_file_path, "a" if append else "w") as file:
        file.write(out_text)
