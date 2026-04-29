/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_core.c

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	#include <windows.h>
#endif

#include "libMultiMarkdown7.h"
#include "text_buffer.h"

#include "mmd_core.h"
#include "mmd_scanner.h"
#include "mmd_node.h"
#include "mmd_line_scanner.h"
#include "mmd_utilities.h"

#include "criticmarkup.h"
#include "transclude.h"

#include "vector_line_node.h"
#include "read_ctx.h"
#include "mmd_parser_hand_2.h"
#include "write_ctx.h"

#include "export_core.h"
#include "ast.h"
#include "docx.h"
#include "epub.h"
#include "html.h"
#include "latex.h"
#include "outline.h"
#include "textbundle.h"

#include "import/html.h"
#include "import/outline.h"
#include "yxml.h"

#ifdef USE_CURL
	#include <curl/curl.h>
#endif

#ifdef TEST
	#include "CuTest.h"
#endif


#define kDEFAULTCAPACITY (4096 * 8)		// How big should file_buffer start?


#define F(i,n) for(int i= 0;i<n;i++)


// https://stackoverflow.com/questions/64893834/measuring-elapsed-time-using-clock-gettimeclock-monotonic
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
static int64_t difftimespec_us(const struct timespec after, const struct timespec before) {
	return ((int64_t)after.tv_sec - (int64_t)before.tv_sec) * (int64_t)1000000
		   + ((int64_t)after.tv_nsec - (int64_t)before.tv_nsec) / 1000;
}
#endif


/// Parse MultiMarkdown text into AST
/// Returns tree of mmd_nodes -- will need to be freed
mmd_node * mmd_parse_filename(const char * fname, read_ctx * c, uint32_t options) {
	mmd_node * n = NULL;
	FILE * in = flex_fopen(fname);

	if (in) {
		n = mmd_parse_file(in, c, options);
		fclose(in);
	}

	return n;
}


mmd_node * mmd_parse_file(FILE * in, read_ctx * c, uint32_t options) {
	text_buffer * buffer = buffer_file(in, kDEFAULTCAPACITY);

	mmd_node * n = mmd_parse_buffer(buffer, c, options);

	text_buffer_free(buffer, 1);

	return n;
}


mmd_node * mmd_parse_str(const char * text, read_ctx * c, uint32_t options) {
	size_t len = strlen(text);
	return mmd_parse_str_len(text, len, c, options);
}


mmd_node * mmd_parse_str_len(const char * text, size_t in_len, read_ctx * c, uint32_t options) {
	// We need to ensure that the text is null-terminated
	text_buffer * buffer = text_buffer_new(in_len + 1);
	text_buffer_append_text(buffer, text, in_len);

	mmd_node * n = mmd_parse_buffer(buffer, c, options);

	text_buffer_free(buffer, 1);

	return n;
}


mmd_node * mmd_parse_buffer(text_buffer * buffer, read_ctx * c, uint32_t options) {
	vector_line_node * vl = vector_line_node_new(0);

	// mmd_node_pool * vn = mmd_node_pool_new(0);
	mmd_node_pool * vn = NULL;

	mmd_node * n = mmd_parse_text(buffer->text, buffer->len, vl, vn, c, options);

	vector_line_node_free(vl);

	return n;
}


/// Parse MultiMarkdown text into AST, and then convert AST into output format
/// Print output to designated FILE stream
void mmd_process_filename(const char * fname, FILE * out, uint32_t options, const char * search_path) {
	FILE * in = flex_fopen(fname);

	if (in) {
		mmd_process_file(in, out, options, search_path, fname);
		fclose(in);
	}
}


void mmd_process_file(FILE * in, FILE * out, uint32_t options, const char * search_path, const char * source_path) {
	text_buffer * source_buffer = buffer_file(in, kDEFAULTCAPACITY);

	text_buffer * out_buffer = text_buffer_new(0);

	mmd_process_buffer(source_buffer, out_buffer, options, search_path, source_path);

	fwrite(out_buffer->text, out_buffer->len, 1, out);

	text_buffer_free(source_buffer, 1);
	text_buffer_free(out_buffer, 1);
}


void mmd_process_str(const char * text, FILE * out, uint32_t options, const char * search_path, const char * source_path) {
	size_t len = strlen(text);
	mmd_process_str_len(text, len, out, options, search_path, source_path);
}


void mmd_process_str_len(const char * text, size_t len, FILE * out, uint32_t options, const char * search_path, const char * source_path) {
	// Copy text in case we modify it (e.g. transclusion)
	text_buffer * source_buffer = text_buffer_new(len);
	text_buffer_append_text(source_buffer, text, len);

	text_buffer * out_buffer = text_buffer_new(0);

	mmd_process_buffer(source_buffer, out_buffer, options, search_path, source_path);

	fwrite(out_buffer->text, out_buffer->len, 1, out);

	text_buffer_free(source_buffer, 1);
	text_buffer_free(out_buffer, 1);
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


void mmd_process_url(const char * url, FILE * out, uint32_t options, const char * search_path, const char * source_path) {
	CURL * curl = curl_easy_init();

	text_buffer * source_buffer = text_buffer_new(0);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) source_buffer);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	curl_easy_setopt(curl, CURLOPT_URL, url);
	CURLcode res = curl_easy_perform(curl);

	if (res == CURLE_OK) {
		// We got it
		text_buffer * out_buffer = text_buffer_new(0);

		mmd_process_buffer(source_buffer, out_buffer, options, search_path, source_path);

		fwrite(out_buffer->text, out_buffer->len, 1, out);

		text_buffer_free(out_buffer, 1);
	} else {
		fprintf(stderr, "There was an error downloading '%s'\n", url);
	}

	text_buffer_free(source_buffer, 1);
	curl_easy_cleanup(curl);
}
#else
void mmd_process_url(const char * url, FILE * out, uint32_t options, const char * search_path, const char * source_path) {
	if (0 && url && out && options && search_path && source_path) {}

	fprintf(stderr, "libcurl is not available.  Unable to download content.\n");
}
#endif



static void mmd_process_buffer_core(vector_line_node * vl, mmd_node_pool * vn, read_ctx * c, text_buffer * source_buffer, text_buffer * out_buffer, uint32_t options, const char * search_path, const char * source_path) {
	if (!(options & MMD_OPTION_COMPATIBILITY)) {
		// Insert `mmdheader` and `mmdfooter` if appropriate
		if (options & MMD_OPTION_MMD_HEADER) {
			switch (MMD_OUT_FORMAT_FROM_OPTS(options)) {
				case FORMAT_MMD:
				case FORMAT_OPML:
				case FORMAT_ITMZ:
					break;

				default:
					mmd_add_mmd_header_footer(source_buffer, options);
					break;
			}
		}

		// Handle transclusion
		if (options & MMD_OPTION_TRANSCLUDE) {
			mmd_transclude(source_buffer, options, search_path, source_path);
		}

		// Accept CriticMarkup
		if (options & MMD_OPTION_CRITIC_ACCEPT) {
			criticmarkup_accept(source_buffer);
		}

		// Reject CriticMarkup
		if (options & MMD_OPTION_CRITIC_REJECT) {
			criticmarkup_reject(source_buffer);
		}
	}

	// If we are exporting raw MMD text, then we are done
	if (MMD_OUT_FORMAT_FROM_OPTS(options) == FORMAT_MMD) {
		text_buffer_append_text(out_buffer, source_buffer->text, source_buffer->len);

		return;
	}

	// Parse MMD into AST
	mmd_node * n = mmd_parse_text(source_buffer->text, source_buffer->len, vl, vn, c, options);

	// Export AST to specified format
	switch (MMD_OUT_FORMAT_FROM_OPTS(options)) {
		case FORMAT_AST:
			export_ast(n, source_buffer->text, out_buffer);
			break;

		case FORMAT_DOCX:
			export_docx(n, source_buffer->text, out_buffer, c, options, source_path);
			break;

		case FORMAT_EPUB:
			export_epub(n, source_buffer->text, out_buffer, c, options, source_path);
			break;

		case FORMAT_HASH: {
			uint32_t hash = mmd_hash_node_tree(n);
			export_hash(n, out_buffer);
			text_buffer_append_printf(out_buffer, "Tree hash: %u\n", hash);
		}
		break;

		case FORMAT_HTML:
			export_html(n, source_buffer->text, out_buffer, c, options, source_path);
			break;

		case FORMAT_ITMZ:
			export_outline(n, source_buffer->text, source_buffer->len, out_buffer, c, options, FORMAT_ITMZ);
			break;

		case FORMAT_BEAMER:
			export_latex(n, source_buffer->text, out_buffer, c, options, FORMAT_BEAMER);
			break;

		case FORMAT_LTX_TALK:
			export_latex(n, source_buffer->text, out_buffer, c, options, FORMAT_LTX_TALK);
			break;

		case FORMAT_LATEX:
			export_latex(n, source_buffer->text, out_buffer, c, options, FORMAT_LATEX);
			break;

		case FORMAT_OPML:
			export_outline(n, source_buffer->text, source_buffer->len, out_buffer, c, options, FORMAT_OPML);
			break;

		case FORMAT_TEXTBUNDLE:
		case FORMAT_TEXTPACK:
			export_textbundle(n, source_buffer, out_buffer, c, options, source_path);
			break;

		default:
			// valid format not found
			break;
	}

	if (options & MMD_OPTION_STATS) {
		fprintf(stderr, "text_buffer %zu/%zu used\n", out_buffer->len, out_buffer->capacity);
		fprintf(stderr, "text_buffer resized %zu times\n", (out_buffer->capacity) / (source_buffer->len * 2));
	}

	if (vn == NULL) {
		// Free nodes if we don't use a node_pool
		mmd_node_tree_free(n);
	}
}


/// All roads lead to Rome....
void mmd_process_buffer(text_buffer * source_buffer, text_buffer * out_buffer, uint32_t options, const char * search_path, const char * source_path) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	// Track time
	struct timespec start, end;

	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#endif

	// Are we starting from OPML or ITMZ?
	if (options & MMD_OPTION_PARSE_OPML) {
		mmd_import_outline(source_buffer, OUTLINE_OPML);
	}

	if (options & MMD_OPTION_PARSE_ITMZ) {
		mmd_import_outline(source_buffer, OUTLINE_ITMZ);
	}

	// Or HTML
	if (options & MMD_OPTION_PARSE_HTML) {
		mmd_import_html(source_buffer);
	}

	// Create structures used for parsing
	vector_line_node * vl = vector_line_node_new(0);
	mmd_node_pool * vn = mmd_node_pool_new(0);

	read_ctx * c = read_ctx_new(options);

	// Parse the text and export it
	mmd_process_buffer_core(vl, vn, c, source_buffer, out_buffer, options, search_path, source_path);

	// Free structures used for parsing
	vector_line_node_free(vl);
	mmd_node_pool_free(vn);
	read_ctx_free(c);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
#endif

	if (options & MMD_OPTION_STATS) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
		int64_t diff_full = difftimespec_us(end, start);
		fprintf(stderr, "%.6f seconds.\n", ((double)diff_full / (double)1000000));
#endif
	}
}


/// Parse MultiMarkdown text into AST, and then convert AST into output format
/// Returns text string (or binary data) -- will need to be freed
char * mmd_process_filename_to_str(const char * fname, size_t * out_len, uint32_t options, const char * search_path) {
	char * out = NULL;
	FILE * in = flex_fopen(fname);

	if (in) {
		out = mmd_process_file_to_str(in, out_len, options, search_path, fname);
		fclose(in);
	}

	return out;
}


char * mmd_process_file_to_str(FILE * in, size_t * out_len, uint32_t options, const char * search_path, const char * source_path) {
	text_buffer * buffer = buffer_file(in, kDEFAULTCAPACITY);

	char * out = mmd_process_buffer_to_str(buffer, out_len, options, search_path, source_path);

	text_buffer_free(buffer, 1);

	return out;
}


char * mmd_process_str_to_str(const char * text, size_t * out_len, uint32_t options, const char * search_path, const char * source_path) {
	size_t len = strlen(text);
	return mmd_process_str_len_to_str(text, len, out_len, options, search_path, source_path);
}


char * mmd_process_str_len_to_str(const char * text, size_t in_len, size_t * out_len, uint32_t options, const char * search_path, const char * source_path) {
	/// Copy text in case we modify it (e.g. transclusion)
	text_buffer * buffer = text_buffer_new(in_len);
	text_buffer_append_text(buffer, text, in_len);

	char * r = mmd_process_buffer_to_str(buffer, out_len, options, search_path, source_path);

	text_buffer_free(buffer, 1);

	return r;
}


char * mmd_process_buffer_to_str(text_buffer * source_buffer, size_t * out_len, uint32_t options, const char * search_path, const char * source_path) {
	text_buffer * out_buffer = text_buffer_new(0);

	mmd_process_buffer(source_buffer, out_buffer, options, search_path, source_path);

	char * result = out_buffer->text;
	*out_len = out_buffer->len;

	text_buffer_free(out_buffer, 0);

	return result;
}


#ifdef USE_CURL
char * mmd_process_url_to_str(const char * url, size_t * out_len, uint32_t options, const char * search_path, const char * source_path) {
	char * result = NULL;
	CURL * curl = curl_easy_init();

	text_buffer * source_buffer = text_buffer_new(0);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) source_buffer);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	curl_easy_setopt(curl, CURLOPT_URL, url);
	CURLcode res = curl_easy_perform(curl);

	if (res == CURLE_OK) {
		// We got it
		text_buffer * out_buffer = text_buffer_new(0);

		mmd_process_buffer(source_buffer, out_buffer, options, search_path, source_path);

		result = out_buffer->text;
		*out_len = out_buffer->len;

		text_buffer_free(out_buffer, 0);
	} else {
		fprintf(stderr, "There was an error downloading '%s'\n", url);
	}

	text_buffer_free(source_buffer, 1);
	curl_easy_cleanup(curl);

	return result;
}
#else
char * mmd_process_url_to_str(const char * url, size_t * out_len, uint32_t options, const char * search_path, const char * source_path) {
	if (0 && url && out_len && options && search_path && source_path) {}

	fprintf(stderr, "libcurl is not available.  Unable to download content.\n");
	return NULL;
}
#endif


/// Process MultiMarkdown text into AST and output it to
/// specified file stream
void mmd_ast_filename(const char * fname, FILE * out, uint32_t options) {
	FILE * in = flex_fopen(fname);

	if (in) {
		mmd_ast_file(in, out, options);
		fclose(in);
	}
}


void mmd_ast_file(FILE * in, FILE * out, uint32_t options) {
	text_buffer * buffer = buffer_file(in, kDEFAULTCAPACITY);

	mmd_ast_buffer(buffer, out, options);

	text_buffer_free(buffer, 1);
}


void mmd_ast_str(const char * text, FILE * out, uint32_t options) {
	size_t len = strlen(text);

	mmd_ast_str_len(text, len, out, options);
}


void mmd_ast_str_len(const char * text, size_t in_len, FILE * out, uint32_t options) {
	// We need to ensure that the text is null-terminated
	text_buffer * buffer = text_buffer_new(in_len + 1);
	text_buffer_append_text(buffer, text, in_len);

	mmd_ast_buffer(buffer, out, options);

	text_buffer_free(buffer, 1);
}


void mmd_ast_buffer(text_buffer * buffer, FILE *out, uint32_t options) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	// Track time
	struct timespec start, mid, end;

	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#endif

	vector_line_node * vl = vector_line_node_new(0);
	mmd_node_pool * vn = mmd_node_pool_new(0);
	read_ctx * c = read_ctx_new(options);

	mmd_node * n = mmd_parse_text(buffer->text, buffer->len, vl, vn, c, options);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	clock_gettime(CLOCK_MONOTONIC_RAW, &mid);
#endif

	mmd_node_tree_describe(n, out, buffer->text, 0);

	vector_line_node_free(vl);
	mmd_node_pool_free(vn);
	read_ctx_free(c);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
#endif

	if (options & MMD_OPTION_STATS) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
		int64_t diff_mid = difftimespec_us(mid, start);
		fprintf(stderr, "%.6f seconds to parse.\n", ((double)diff_mid / (double)1000000));

		int64_t diff_full = difftimespec_us(end, start);
		fprintf(stderr, "%.6f seconds in total.\n", ((double)diff_full / (double)1000000));
#endif
	}
}


/// Process MultiMarkdown text into AST with hash values and output it to
/// specified file stream
void mmd_hash_filename(const char * fname, FILE * out, uint32_t options) {
	FILE * in = flex_fopen(fname);

	if (in) {
		mmd_hash_file(in, out, options);
		fclose(in);
	}
}


void mmd_hash_file(FILE * in, FILE * out, uint32_t options) {
	text_buffer * buffer = buffer_file(in, kDEFAULTCAPACITY);

	mmd_hash_buffer(buffer, out, options);

	text_buffer_free(buffer, 1);
}


void mmd_hash_str(const char * text, FILE * out, uint32_t options) {
	size_t len = strlen(text);

	mmd_hash_str_len(text, len, out, options);
}


void mmd_hash_str_len(const char * text, size_t in_len, FILE * out, uint32_t options) {
	// We need to ensure that the text is null-terminated
	text_buffer * buffer = text_buffer_new(in_len + 1);
	text_buffer_append_text(buffer, text, in_len);

	mmd_hash_buffer(buffer, out, options);

	text_buffer_free(buffer, 1);
}


void mmd_hash_buffer(text_buffer * buffer, FILE * out, uint32_t options) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	// Track time
	struct timespec start, mid, end;

	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#endif

	vector_line_node * vl = vector_line_node_new(0);
	mmd_node_pool * vn = mmd_node_pool_new(0);
	read_ctx * c = read_ctx_new(options);

	mmd_node * n = mmd_parse_text(buffer->text, buffer->len, vl, vn, c, options);
	uint32_t hash = mmd_hash_node_tree(n);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	clock_gettime(CLOCK_MONOTONIC_RAW, &mid);
#endif

	mmd_node_tree_describe_hash(n, out);
	fprintf(out, "Tree hash: %u\n", hash);

	vector_line_node_free(vl);
	mmd_node_pool_free(vn);
	read_ctx_free(c);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
#endif

	if (options & MMD_OPTION_STATS) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
		int64_t diff_mid = difftimespec_us(mid, start);
		fprintf(stderr, "%.6f seconds to parse.\n", ((double)diff_mid / (double)1000000));

		int64_t diff_full = difftimespec_us(end, start);
		fprintf(stderr, "%.6f seconds in total.\n", ((double)diff_full / (double)1000000));
#endif
	}
}


read_ctx * mmd_metadata_filename(const char * fname, uint32_t options) {
	FILE * in = flex_fopen(fname);

	read_ctx * r = NULL;

	if (in) {
		r = mmd_metadata_file(in, options);
		fclose(in);
	}

	return r;
}


read_ctx * mmd_metadata_file(FILE * in, uint32_t options) {
	text_buffer * buffer = buffer_file(in, kDEFAULTCAPACITY);

	read_ctx * r = mmd_metadata_buffer(buffer, options);

	text_buffer_free(buffer, 1);

	return r;
}


read_ctx * mmd_metadata_str(const char * text, uint32_t options) {
	size_t len = strlen(text);
	return mmd_metadata_str_len(text, len, options);
}


read_ctx * mmd_metadata_str_len(const char * text, size_t in_len, uint32_t options) {
	// We need to ensure that the text is null-terminated
	text_buffer * buffer = text_buffer_new(in_len + 1);
	text_buffer_append_text(buffer, text, in_len);

	read_ctx * r = mmd_metadata_buffer(buffer, options);

	text_buffer_free(buffer, 1);

	return r;
}


read_ctx * mmd_metadata_buffer(text_buffer * buffer, uint32_t options) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	// Track time
	struct timespec start, mid, end;

	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#endif

	vector_line_node * vl = vector_line_node_new(0);
	mmd_node_pool * vn = mmd_node_pool_new(0);
	read_ctx * r = read_ctx_new(options);

	mmd_parse_metadata(buffer->text, buffer->len, vl, vn, r, options);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	clock_gettime(CLOCK_MONOTONIC_RAW, &mid);
#endif

	vector_line_node_free(vl);
	mmd_node_pool_free(vn);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
#endif

	if (options & MMD_OPTION_STATS) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
		int64_t diff_mid = difftimespec_us(mid, start);
		fprintf(stderr, "%.6f seconds to parse.\n", ((double)diff_mid / (double)1000000));

		int64_t diff_full = difftimespec_us(end, start);
		fprintf(stderr, "%.6f seconds in total.\n", ((double)diff_full / (double)1000000));
#endif
	}

	return r;
}


read_ctx * mmd_tags_filename(const char * fname, uint32_t options) {
	FILE * in = flex_fopen(fname);

	read_ctx * r = NULL;

	if (in) {
		r = mmd_tags_file(in, options);
		fclose(in);
	}

	return r;
}


read_ctx * mmd_tags_file(FILE * in, uint32_t options) {
	text_buffer * buffer = buffer_file(in, kDEFAULTCAPACITY);

	read_ctx * r = mmd_tags_buffer(buffer, options);

	text_buffer_free(buffer, 1);

	return r;
}


read_ctx * mmd_tags_str(const char * text, uint32_t options) {
	size_t len = strlen(text);
	return mmd_tags_str_len(text, len, options);
}


read_ctx * mmd_tags_str_len(const char * text, size_t in_len, uint32_t options) {
	// We need to ensure that the text is null-terminated
	text_buffer * buffer = text_buffer_new(in_len + 1);
	text_buffer_append_text(buffer, text, in_len);

	read_ctx * r = mmd_tags_buffer(buffer, options);

	text_buffer_free(buffer, 1);

	return r;
}


read_ctx * mmd_tags_buffer(text_buffer * buffer, uint32_t options) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	// Track time
	struct timespec start, mid, end;

	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#endif

	vector_line_node * vl = vector_line_node_new(0);
	mmd_node_pool * vn = mmd_node_pool_new(0);
	read_ctx * r = read_ctx_new(options);

	mmd_parse_text(buffer->text, buffer->len, vl, vn, r, options);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	clock_gettime(CLOCK_MONOTONIC_RAW, &mid);
#endif

	vector_line_node_free(vl);
	mmd_node_pool_free(vn);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
#endif

	if (options & MMD_OPTION_STATS) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
		int64_t diff_mid = difftimespec_us(mid, start);
		fprintf(stderr, "%.6f seconds to parse.\n", ((double)diff_mid / (double)1000000));

		int64_t diff_full = difftimespec_us(end, start);
		fprintf(stderr, "%.6f seconds in total.\n", ((double)diff_full / (double)1000000));
#endif
	}

	return r;
}
