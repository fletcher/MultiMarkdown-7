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


#include <stdlib.h>

#include "zip.h"


// Windows deprecated mkdir()
// Fix per internet searches and modified by @f8ttyc8t (<https://github.com/f8ttyc8t>)
#if (defined(_WIN32) || defined(__WIN32__))
	// Let compiler know where to find _mkdir()
	#include  <direct.h>
	#define mkdir(A, B) _mkdir(A)
#endif



/// Create a new zip archive
mz_bool zip_new_archive(mz_zip_archive * pZip) {
	memset(pZip, 0, sizeof(mz_zip_archive));

	mz_bool status;

	status = mz_zip_writer_init_heap(pZip, 0, 0);

	if (!status) {
		fprintf(stderr, "mz_zip_writer_init_heap() failed.\n");
	}

	return status;
}

