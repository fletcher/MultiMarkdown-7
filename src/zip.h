/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file zip.h

	@brief


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


#ifndef ZIP_LIBMULTIMARKDOWN7_H
#define ZIP_LIBMULTIMARKDOWN7_H

#include "miniz.h"
#include "text_buffer.h"

/// Create a new zip archive
mz_bool zip_new_archive(mz_zip_archive * pZip);
mz_bool zip_extract_to_path(mz_zip_archive * pZip, const char * path);
mz_bool zip_binary_extract_to_path(const char * data, size_t len, const char * path);

mz_bool zip_extract_file(mz_zip_archive * pZip, const char * fname, text_buffer * out);
mz_bool zip_binary_extract_file(const char * data, size_t len, const char * fname, text_buffer * out);

#endif
