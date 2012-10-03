// mmap_util.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Utility wrappers for memory-mapping functionality on Posix and
// Windows.


#include "mmap_util.h"


#ifdef WIN32


#include <windows.h>
#include <stdio.h>
#include "engine/container.h"


namespace mmap_util {

	hash<void*, char*>	s_temp_filenames;


	void*	map(int size, bool writeable, const char* filename)
	{
		bool	create_file = false;

		if (filename == NULL) {
			// Generate a temporary file name.
			filename = _tempnam(".", "tmp");
			create_file = true;

			printf("temp file = %s\n", filename);//xxxxxx
		}

		HANDLE	filehandle = CreateFile(filename,
						GENERIC_READ | (writeable ? GENERIC_WRITE : 0),
						FILE_SHARE_READ,
						NULL,
						OPEN_ALWAYS,
						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS | (create_file ? FILE_ATTRIBUTE_TEMPORARY : 0),
						NULL);
		if (filehandle == INVALID_HANDLE_VALUE) {
			// Failure.
			throw "can't CreateFile";

			if (create_file) {
				// _tempnam malloc's the filename.
				free((void*) filename);
			}

			return NULL;
		}

		if (create_file) {
			// Expand the new file to 'size' bytes.
			SetFilePointer(filehandle, size, NULL, FILE_BEGIN);
			SetEndOfFile(filehandle);
		}

		HANDLE	file_mapping = CreateFileMapping(filehandle, NULL, writeable ? PAGE_READWRITE : PAGE_READONLY, 0, 0, NULL);
		if (file_mapping == NULL) {
			// Failure.
			CloseHandle(filehandle);

			if (create_file) {
				_unlink(filename);
				// _tempnam malloc's the filename.
				free((void*) filename);
			}

			printf("mmap_util::map(): CreateFileMapping failed.\n");

			return NULL;
		}

		void*	data = MapViewOfFile(file_mapping, writeable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, size);
		if (data == NULL) {
			printf("mmap_util::map(): MapViewOfFile failed.  size = %d\n", size);

			LPVOID lpMsgBuf;
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			printf("%s\n", lpMsgBuf);
			LocalFree(lpMsgBuf);
		}

		CloseHandle(file_mapping);
		CloseHandle(filehandle);

		if (create_file) {
			if (data) {
				// Remember the filename so we can delete it & free the name later.
				s_temp_filenames.add(data, (char*) filename);
			} else {
				_unlink(filename);
				free((void*) filename);
			}
		}

		return data;
	}


	void	unmap(void* data, int size)
	{
		UnmapViewOfFile(data);

		char*	filename = NULL;
		if (s_temp_filenames.get(data, &filename)) {
			// Need to delete this temporary file.
			_unlink(filename);

			// _tempnam malloc's the filename.
			free((void*) filename);

			// @@ need to remove (data, filename) from s_temp_filenames!
		}
	}
};


#else // not WIN32


// (implementation & debugging from tbp)


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cassert>

#include "engine/container.h"



// wrappers for mmap/munmap
namespace mmap_util {
	hash<void*, char*>	s_temp_filenames;

	// Create a memory-mapped window into a file.  If filename is
	// NULL, create a temporary file for the data.  If a valid
	// filename is specified, then open the file and use it as the
	// data.  On success, returns a pointer to the beginning of the
	// memory-mapped area.  Returns NULL on failure.
	//
	// Use unmap() to release the mapping.
	//
	// The writable param specifies whether the returned buffer can be
	// written to.  You may get better performance with a read-only
	// buffer.
	void*	map(int size, bool writeable, const char* filename)
	{
		assert(size > 0);

		int fildes;
		bool created = false;
		char *tmpname = NULL;
		void* data = NULL;
		if (filename == NULL) {
			// make a temporary file.
			tmpname  = new char[64];
			strcpy(tmpname, "/tmp/chunker-XXXXXX");
			fildes = mkstemp(tmpname);	
			if (fildes == -1) {
				return NULL;
			} else {
				created = true;
			}
	
			// Force file growing
			if (lseek(fildes,size,SEEK_SET) == -1) {
				goto UNWIND;
			}
			write(fildes,'\0',1)/* == -1 */;
		}
		// else if size == 0 then size = filesize(filename);
		else {
			fildes = open(filename, (writeable ? O_RDWR : O_RDONLY) | O_CREAT);
			if (fildes == -1) {
				// Failure.
				return NULL;
			}
			size = lseek(fildes,0,SEEK_END);
			if (size <= 0) {
				goto UNWIND;
			}
		}
		lseek(fildes,0,SEEK_SET);
		data = mmap(0, size, PROT_READ | (writeable ? PROT_WRITE : 0), MAP_SHARED, fildes, 0);

		if (data == (void*) -1) {
			// Failure.
			goto UNWIND;
		}
		else if (created) {
			s_temp_filenames.add(data, tmpname);
		}
		return data;

	UNWIND: // No execeptions => gotos ;)
		perror("mmap_util::map() -- ");
		// At least opened at this point
		close(fildes);
		if (created) {
			unlink(tmpname);
			delete tmpname;
		}
		return NULL;
	}
	     

	// Unmap a file mapped via map().
	void	unmap(void* data, int size)
	{
		munmap(data, size);
		char*	filename = NULL;
		if (s_temp_filenames.get(data, &filename)) {
			// Need to delete this temporary file.
			unlink(filename);
			delete filename;

			// @@ need to remove (data, filename) from s_temp_filenames!
		}			
	}
};


#endif // not WIN32

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
