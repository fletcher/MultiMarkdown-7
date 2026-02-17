/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file assets.c

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


#include <stdlib.h>

#include "libMultiMarkdown.h"
#include "assets.h"
#include "text_buffer.h"
#include "transclude.h"
#include "mmd_utilities.h"


mz_bool archive_asset_from_file(mz_zip_archive * pZip, const char * destination, const char * fname, const char * directory) {
	mz_bool status = 0;

	if (pZip && destination && fname && directory) {
		char * path = concatenate_paths(directory, fname, true);

		FILE * in = flex_fopen(path);

		if (in) {
			text_buffer * buffer = buffer_file(in, 8192);

			status = mz_zip_writer_add_mem(pZip, destination, buffer->text, buffer->len, MZ_BEST_COMPRESSION);

			text_buffer_free(buffer, true);
		}
	}

	return status;
}


#ifdef USE_CURL


#else

/// Add assets to zip archive from a local directory
mz_bool archive_assets(mz_zip_archive * pZip, read_ctx * r, const char * destination, const char * directory) {
	mz_bool status = 0;

	if (pZip && r && destination && directory) {
		asset * a, * a_tmp;
		status = 1;

		HASH_ITER(hh, r->asset_hash, a, a_tmp) {
			fprintf(stderr, "Store asset %s => %s\n", a->uuid, a->url);
			char * target = concatenate_paths(destination, a->uuid, false);

			if (!archive_asset_from_file(pZip, target, a->url, directory)) {
				status = 0;
			}

			free(target);
		}
	}

	return status;
}

#endif

