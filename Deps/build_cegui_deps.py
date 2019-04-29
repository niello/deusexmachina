import os
import sys
import shutil
from subprocess import check_call
from hashlib import sha256

import requests
import zipfile

PROJECT_FOLDER = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))


def unpack_repo(archive_path, dest_folder, has_root_dir):
	# Clear destination
	if os.path.exists(dest_folder):
		shutil.rmtree(dest_folder)
	os.mkdir(dest_folder)

	if has_root_dir:
		# [Re]create tmp directory
		path_to_tmp = os.path.join(os.path.dirname(archive_path), 'tmp')
		if os.path.exists(path_to_tmp):
			shutil.rmtree(path_to_tmp)
		os.mkdir(path_to_tmp)

		# Unpack zip to tmp
		with zipfile.ZipFile(archive_path, 'r') as zip_ref:
			zip_ref.extractall(path_to_tmp)

		# Move from the only tmp subdirectory to root
		for subname in os.listdir(path_to_tmp):
			full_subpath = os.path.join(path_to_tmp, subname)
			if os.path.isdir(full_subpath):
				for repo_data in os.listdir(full_subpath):
					shutil.move(os.path.join(full_subpath, repo_data), dest_folder)

		# Remove tmp
		shutil.rmtree(path_to_tmp)
	else:
		# Unpack directly
		with zipfile.ZipFile(archive_path, 'r') as zip_ref:
			zip_ref.extractall(dest_folder)


if __name__ == "__main__":
	cmake_generator_name = sys.argv[1] if len(sys.argv) > 1 else 'Visual Studio 15 2017'

	print("Building CEGUI dependencies using CMake generator " + cmake_generator_name)

	expected_archive_sha256 = "b1c822d880b3d361782f1807ea8a6f55e0834a02afd983f2a5141db04a333938"
	downloads_folder = os.path.join(PROJECT_FOLDER, 'Deps', 'Downloads')
	archive_path = os.path.join(downloads_folder, 'CEGUI-Deps.zip')

	# Check actuality of downloaded CEGUI dependency archive
	need_downloading = True
	if os.path.exists(archive_path):
		with open(archive_path, 'rb') as arch:
			existing_sha256 = sha256(arch.read()).hexdigest()
			if existing_sha256 == expected_archive_sha256:
				print("SHA-256 hashes match, skip downloading")
				need_downloading = False
		if need_downloading:
			os.remove(archive_path)

	# Download archive
	if need_downloading:
		deps_url = 'https://bitbucket.org/Jahndal/cegui-dependencies/get/dem/default-minimal.zip'
		print("Downloading CEGUI-Deps from " + deps_url)
		r = requests.get(deps_url, allow_redirects=True)
		downloaded_sha256 = sha256(r.content).hexdigest()
		if downloaded_sha256 != expected_archive_sha256:
			print("Error: CEGUI dependencies archive SHA-256 (" + downloaded_sha256 + ") doesn't match expected value")
			sys.exit(1)

		if not os.path.exists(downloads_folder):
			os.mkdir(downloads_folder)
		with open(archive_path, 'wb') as arch:
			arch.write(r.content)

	# Unpack repo to an expected location and clear temporary data
	dest_folder = os.path.join(PROJECT_FOLDER, 'Deps', 'CEGUI-Deps')
	unpack_repo(archive_path, dest_folder, True)

	build_folder = os.path.join(dest_folder, 'build')

	check_call(['cmake', '-G', cmake_generator_name, '-DCMAKE_CONFIGURATION_TYPES:STRING=Debug;Release', '-S', dest_folder, '-B' + build_folder])
	check_call(['cmake', '--build', build_folder, '--target', 'ALL_BUILD', '--config', 'Debug'])
	check_call(['cmake', '--build', build_folder, '--target', 'ALL_BUILD', '--config', 'Release'])
