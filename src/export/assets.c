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

#include "libMultiMarkdown7.h"
#include "assets.h"
#include "transclude.h"
#include "mmd_utilities.h"


#ifdef __APPLE__
	#include "TargetConditionals.h"
	#if TARGET_IPHONE_SIMULATOR
		// iOS Simulator
		#undef USE_CURL
	#elif TARGET_OS_IPHONE
		// iOS device
		#undef USE_CURL
	#elif TARGET_OS_MAC
		// Other kinds of Mac OS
	#else
		#error "Unknown Apple platform"
	#endif
#endif

#ifdef USE_CURL
	#include <curl/curl.h>
#endif


/// Archive the file at specified fname and directory
mz_bool archive_asset_from_file(mz_zip_archive * pZip, const char * destination, const char * fname, const char * directory) {
	mz_bool status = 0;

	if (pZip && destination && fname && directory) {
		char * path = concatenate_paths(directory, fname, true);

		FILE * in = flex_fopen(path);

		free(path);

		if (in) {
			text_buffer * buffer = buffer_file(in, 8192);

			status = mz_zip_writer_add_mem(pZip, destination, buffer->text, buffer->len, MZ_BEST_COMPRESSION);

			text_buffer_free(buffer, true);

			fclose(in);
		}
	}

	return status;
}


int asset_load_local(asset * a, const char * source_path) {
	if (a && source_path) {
		char * path = concatenate_paths(source_path, a->url, true);

		FILE * in = flex_fopen(path);

		free(path);

		if (in) {
			text_buffer * buffer = buffer_file(in, 8192);

			if (buffer) {
				a->data = buffer->text;
				a->len = buffer->len;
				text_buffer_free(buffer, 0);
				a->stored = 1;
			}

			fclose(in);
		}

		return a->stored;
	}

	return 0;
}


#ifdef USE_CURL

// Use dynamic buffer for downloading files in memory
// Based on https://curl.haxx.se/libcurl/c/getinmemory.html

static size_t write_memory(void * contents, size_t size, size_t nmemb, void * userp) {
	text_buffer * buffer = (text_buffer *) userp;
	size_t startlen = buffer->len;

	text_buffer_append_text(buffer, contents, (size * nmemb));

	return buffer->len - startlen;
}


/// Store asset data using curl
int asset_load_with_curl(asset * a) {
	if (a && a->data == NULL) {
		CURL * curl = curl_easy_init();

		text_buffer * buffer = text_buffer_new(0);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) buffer);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		curl_easy_setopt(curl, CURLOPT_URL, a->url);
		CURLcode res = curl_easy_perform(curl);

		if (res == CURLE_OK) {
			// We got it
			a->data = buffer->text;
			a->len = buffer->len;
			text_buffer_free(buffer, 0);
			a->stored = 1;

			return 1;
		} else {
			text_buffer_free(buffer, 1);
		}

		curl_easy_cleanup(curl);
	}

	return 0;
}


/// Add assets to zip archive using curl
mz_bool archive_asset_with_curl(mz_zip_archive * pZip, const char * destination, const char * url) {
	mz_bool status = 0;

	if (pZip && destination && url) {
		CURL * curl = curl_easy_init();

		text_buffer * buffer = text_buffer_new(0);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) buffer);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		curl_easy_setopt(curl, CURLOPT_URL, url);
		CURLcode res = curl_easy_perform(curl);

		if (res == CURLE_OK) {
			// We got it
			status = mz_zip_writer_add_mem(pZip, destination, buffer->text, buffer->len, MZ_BEST_COMPRESSION);

			if (!status) {
				fprintf(stderr, "Failed to archive downloaded file.\n");
			}
		}

		text_buffer_free(buffer, 1);
		curl_easy_cleanup(curl);
	}

	return status;
}


/// Add assets to zip archive via downloading
mz_bool archive_assets_to_zip(mz_zip_archive * pZip, read_ctx * r, const char * destination, const char * directory, uint32_t options) {
	mz_bool status = 1;

	if (pZip && r && destination) {
		asset * a, * a_tmp;

		HASH_ITER(hh, r->asset_hash, a, a_tmp) {
			char * target = concatenate_paths(destination, a->uuid, false);

			if (options & MMD_OPTION_DOWNLOAD_ASSETS) {
				if (!archive_asset_with_curl(pZip, target, a->url)) {
					// Unable to get via curl -- attempt locally
					if (directory) {
						if (!archive_asset_from_file(pZip, target, a->url, directory)) {
							fprintf(stderr, "Failed to download '%s'. Not available locally.\n", a->url);
							status = 0;
						} else {
							a->stored = 1;
						}
					} else {
						fprintf(stderr, "Failed to download '%s'. No local directory specified.\n", a->url);
						status = 0;
					}
				} else {
					a->stored = 1;
				}
			} else {
				if (!archive_asset_from_file(pZip, target, a->url, directory)) {
					fprintf(stderr, "Unable to archive asset '%s' from local directory.\n", a->url);
					status = 0;
				} else {
					a->stored = 1;
				}
			}

			free(target);
		}
	}

	return status;
}


#else

/// Add assets to zip archive from a local directory (curl is not available)
mz_bool archive_assets_to_zip(mz_zip_archive * pZip, read_ctx * r, const char * destination, const char * directory, uint32_t options) {
	mz_bool status = 0;

	if (pZip && r && destination && directory) {
		asset * a, * a_tmp;
		status = 1;

		HASH_ITER(hh, r->asset_hash, a, a_tmp) {
			char * target = concatenate_paths(destination, a->uuid, false);

			if (!archive_asset_from_file(pZip, target, a->url, directory)) {
				fprintf(stderr, "Unable to archive asset '%s'\n", a->url);
				status = 0;
			} else {
				a->stored = 1;
			}

			free(target);
		}
	}

	// Silence unused parameter warning
	if (options == 0) {
		options = 0;
	}

	return status;
}

#endif


/// Store asset data
void asset_store_data(asset * a, uint32_t options, const char * source_path) {
	if (a->stored) {
		// Already done
		return;
	}

	// Use libcurl to download from internet?
	if (options & MMD_OPTION_DOWNLOAD_ASSETS) {
#ifdef USE_CURL

		if (asset_load_with_curl(a)) {
			return;
		}

#endif
	}

	// Look for local file
	asset_load_local(a, source_path);
}
