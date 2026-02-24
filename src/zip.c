/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file zip.c

	@brief Wrapper for miniz with a couple of common routines


	@author	Fletcher T. Penney
	@bug

**/

/*

	MIT License

	Copyright (c) 2024-2026 Fletcher T. Penney

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

*/


#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	#include <windows.h>
#else
	#include <dirent.h>
	#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "text_buffer.h"
#include "zip.h"


// Windows deprecated mkdir()
// Fix per internet searches and modified by @f8ttyc8t (<https://github.com/f8ttyc8t>)
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	// Let compiler know where to find _mkdir()
	#include  <direct.h>
	#define mkdir(A, B) _mkdir(A)
#endif


#define F(i,n) for(int i= 0;i<n;i++)


/// Create a new zip archive
mz_bool zip_new_archive(mz_zip_archive * pZip) {
	memset(pZip, 0, sizeof(mz_zip_archive));

	mz_bool status;

	status = mz_zip_writer_init_heap(pZip, 0, sizeof(mz_zip_archive));

	if (!status) {
		fprintf(stderr, "mz_zip_writer_init_heap() failed.\n");
	}

	return status;
}


static int directory_exists(const char * path) {
	struct stat status;

	if (stat(path, &status) == 0 && (status.st_mode & S_IFDIR)) {
		return 1;
	}

	return 0;
}


static int file_exists(const char * path) {
	struct stat status;

	if (stat(path, &status) == 0 && !(status.st_mode & S_IFDIR)) {
		return 1;
	}

	return 0;
}


mz_bool zip_extract_to_path(mz_zip_archive * pZip, const char * path) {
	mz_bool status = 1;


	if (!directory_exists(path)) {
		// path is not an existing directory

		if (file_exists(path)) {
			fprintf(stderr, "'%s' is an existing file, not a directory.\n", path);
			return 0;
		} else {
			// Path doesn't exist, create directory
			mkdir(path, 0755);
		}
	}

	if (directory_exists(path)) {
		// Change working directory
		char cwd[4097];

		if (getcwd(cwd, sizeof(cwd))) {

		}

		// Move into desired path
		if (chdir(path)) {

		}

		int file_count = mz_zip_reader_get_num_files(pZip);

		mz_zip_archive_file_stat pStat;

		F(i, file_count) {
			mz_zip_reader_file_stat(pZip, i, &pStat);

			if (pStat.m_is_directory) {
				mkdir(pStat.m_filename, 0755);
			} else {
				if (!mz_zip_reader_extract_to_file(pZip, i, pStat.m_filename, 0)) {
					fprintf(stderr, "Error extracting '%s' from zip archive.\n", pStat.m_filename);
					status = 0;
				}
			}
		}

		if (chdir(cwd)) {

		}
	}

	return status;
}


mz_bool zip_binary_extract_to_path(const char * data, size_t len, const char * path) {
	mz_zip_archive pZip = {0};
	mz_bool status = mz_zip_reader_init_mem(&pZip, data, len, 0);

	if (status) {
		status = mz_zip_validate_archive(&pZip, 0);

		if (status) {
			status = zip_extract_to_path(&pZip, path);
		} else {
			fprintf(stderr, "mz_zip_validate_archive() failed.\n");
		}
	} else {
		fprintf(stderr, "mz_zip_reader_init_mem() failed.\n");
	}

	mz_zip_reader_end(&pZip);
	return status;
}


mz_bool zip_extract_file(mz_zip_archive * pZip, const char * fname, text_buffer * out) {
	mz_uint32 index;
	mz_zip_archive_file_stat pStat;
	mz_bool status;

	status = mz_zip_reader_locate_file_v2(pZip, fname, NULL, 0, &index);

	if (status == -1) {
		fprintf(stderr, "mz_zip_reader_locate_file_v2() unable to find '%s'\n", fname);
		return 0;
	}

	mz_zip_reader_file_stat(pZip, index, &pStat);
	size_t size = pStat.m_uncomp_size + 1;	// Allow for null terminator

	if (out->capacity < size) {
		free(out->text);
		out->text = malloc(size);
		out->capacity = size;
		out->len = 0;
	}

	status = mz_zip_reader_extract_to_mem(pZip, index, out->text, out->capacity, 0);

	if (status) {
		out->len = size - 1;
		out->text[out->len] = '\0';
	} else {
		fprintf(stderr, "mz_zip_reader_extract_to_mem() failed\n");
	}

	return status;
}


mz_bool zip_binary_extract_file(const char * data, size_t len, const char * fname, text_buffer * out) {
	mz_zip_archive pZip = {0};
	mz_bool status = mz_zip_reader_init_mem(&pZip, data, len, 0);

	if (status) {
		status = mz_zip_validate_archive(&pZip, 0);

		if (status) {
			status = zip_extract_file(&pZip, fname, out);
		} else {
			fprintf(stderr, "mz_zip_validate_archive() failed.\n");
		}
	} else {
		fprintf(stderr, "mz_zip_reader_init_mem() failed.\n");
	}

	mz_zip_reader_end(&pZip);
	return status;
}
