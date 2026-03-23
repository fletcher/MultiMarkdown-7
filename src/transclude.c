/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file transclude.c

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

#include "libMultiMarkdown7.h"

#include "text_buffer.h"
#include "mmd_core.h"
#include "read_ctx.h"
#include "transclude.h"
#include "stack.h"
#include "mmd_utilities.h"


#ifdef TEST
	#include "CuTest.h"
#endif


#define F(i,n) for(int i= 0;i<n;i++)


// Windows does not know realpath(), so we need a "windows port"
// Fix by @f8ttyc8t (<https://github.com/f8ttyc8t>)
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
// Let compiler know where to find GetFullPathName()
#include <windows.h>

char * realpath(const char * path, char * resolved_path) {
	DWORD  retval = 0;
	DWORD  dwBufSize = 0; // Just in case MAX_PATH differs from PATH_MAX
	TCHAR * buffer = NULL;

	if (resolved_path == NULL) {
		// realpath allocates appropiate bytes if resolved_path is null. This is to mimic realpath behavior
		dwBufSize = MAX_PATH; // Use windows MAX_PATH constant, because we are in Windows context now.
		buffer = (char *)malloc(dwBufSize);

		if (buffer == NULL) {
			return NULL; // some really weird is going on...
		}
	} else {
		dwBufSize = MAX_PATH;  // buffer has been allocated using MAX_PATH earlier
		buffer = resolved_path;
	}

	retval = GetFullPathName(path, dwBufSize, buffer, NULL);

	if (retval == 0) {
		return NULL;
		printf("Failed to GetFullPathName()\n");
	}

	return buffer;
}

#endif


/// Prepend `mmdheader` and append `mmdfooter` metadata to document content for processing
void mmd_add_mmd_header_footer(text_buffer * buffer, uint32_t options) {
	read_ctx * r = mmd_metadata_buffer(buffer, options);

	if (r) {
		meta * m = read_ctx_get_meta(r, "mmdheader");

		if (m) {
			// Ensure space between metadata and inserted content (it's ok if this is not needed)
			text_buffer_insert_text(buffer, r->meta_end, "\n\n", 2);
			text_buffer_insert_text(buffer, r->meta_end + 2, m->value, strlen(m->value));
		}

		m = read_ctx_get_meta(r, "mmdfooter");

		if (m) {
			// Ensure space between metadata and inserted content (it's ok if this is not needed)
			text_buffer_append_text(buffer, "\n\n", 2);
			text_buffer_append_text(buffer, m->value, strlen(m->value));
		}
	}

	read_ctx_free(r);
}


/// Windows can use either `\` or `/` as a separator -- thanks to t-beckmann on github
///	for suggesting a fix for this.
int is_separator(char c) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	return c == '\\' || c == '/';
#else
	return c == '/';
#endif
}


#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	#define separator_char '\\'
#else
	#define separator_char '/'
#endif


char * concatenate_paths(const char * dir, const char * path, int resolve) {
	int len = (int) (strlen(dir) + 1 + strlen(path) + 1);

	char * temp = malloc(sizeof(char) * len);

	if (is_separator(dir[strlen(dir) - 1])) {
		snprintf(temp, len, "%s%s", dir, path);
	} else {
		snprintf(temp, len, "%s%c%s", dir, separator_char, path);
	}

	if (resolve) {
		char * r = realpath(temp, NULL);
		free(temp);
		return r;
	} else {
		return temp;
	}
}


read_ctx * mmd_transclude_recursive(text_buffer * buffer, uint32_t options, const char * search_path, const char * source_path, stack * parsed) {
	stack_push(parsed, (void *) source_path);

	char * start, * stop;
	size_t offset = 0;
	int free_search = 0;

	// We don't want to transclude inside metadata
	read_ctx * r = mmd_metadata_buffer(buffer, options);

	if (r) {
		offset = r->meta_end;

		// Is there an override for the search_path?
		meta * m = read_ctx_get_meta(r, "transcludebase");

		if (m && m->value) {
			if (is_separator(m->value[0])) {
				// Absolute path
				search_path = m->value;
			} else {
				// Path is relative to the document
				char * dir = mmd_dirname(source_path);
				search_path = concatenate_paths(dir, m->value, true);
				free(dir);
				free_search = 1;
			}
		}
	}

	// Iterate through buffer, looking for `{{...}}`
	start = strstr(&buffer->text[offset], "{{");

	while (start && start < buffer->text + buffer->len) {
		stop = strstr(start, "}}");

		if (stop == NULL || stop > buffer->text + buffer->len) {
			break;
		}

		// Ensure valid match
		if (!strncmp("{{TOC}}", start, 7)) {
			start = strstr(stop, "{{");
			continue;
		}

		// Clean up and see if file exists
		char * file_path = NULL;

		if (is_separator(start[2])) {
			// Absolute path
			int len = (int) (stop - start - 2 + 10);
			file_path = malloc(sizeof(char) * len);
			snprintf(file_path, len, "%.*s", (int)(stop - start - 2), &start[2]);
		} else {
			// Relative path
			int len = (int) (strlen(search_path) + (stop - start) - 2 + 10);
			file_path = malloc(sizeof(char) * len);
			snprintf(file_path, len, "%s%c%.*s", search_path, separator_char, (int)(stop - start - 2), &start[2]);
		}

		// Wildcard?
		if (MMD_OUT_FORMAT_FROM_OPTS(options) != FORMAT_MMD && (strncmp((stop - 2), ".*", 2) == 0)) {
			file_path[strlen(file_path) - 2] = '\0';

			switch (MMD_OUT_FORMAT_FROM_OPTS(options)) {
				case FORMAT_HTML:
				case FORMAT_EPUB:
					strcat(file_path, ".html");
					break;

				case FORMAT_LATEX:
				case FORMAT_BEAMER:
				case FORMAT_LTX_TALK:
					strcat(file_path, ".tex");
					break;

				case FORMAT_FODT:
				case FORMAT_ODT:
					strcat(file_path, ".fodt");
					break;

				default:
					strcat(file_path, ".txt");
					break;
			}
		}

		char * clean_path = realpath(file_path, NULL);

		if (clean_path == NULL) {
			goto finish_match;
		}

		// Prevent infinite loops
		F(i, (int)parsed->size) {
			char * t = stack_peek_index(parsed, i);

			if (t && strcmp(clean_path, t) == 0) {
				// We've alread parsed this file
				goto finish_match;
			}
		}

		// Perform transclusion
		text_buffer * buf = buffer_filename(clean_path, 0);

		if (buf) {
			// Strip BOM
			text_buffer_delete_bom(buf);

			read_ctx * rr = mmd_transclude_recursive(buf, options, search_path, clean_path, parsed);

			// We don't want to insert metadata from transcluded file
			text_buffer_delete_range(buf, 0, rr->meta_end);

			text_buffer_replace_range(buffer, start - buffer->text, stop - start + 2, buf->text, buf->len);

			// Skip over new text to continue searching for transclusion tokens
			stop = start + buf->len;

			text_buffer_free(buf, 1);
			read_ctx_free(rr);
		}

finish_match:

		free(file_path);
		free(clean_path);
		start = strstr(stop, "{{");
	}

	stack_pop(parsed);

	if (free_search) {
		free((void *)search_path);
	}

	return r;
}


void mmd_transclude(text_buffer * buffer, uint32_t options, const char * search_path, const char * source_path) {
	// Ensure source_path is an absolute path
	char * absolute_source_path = NULL;
	char * absolute_search_path = NULL;
	char * source_copy = NULL;

	if (source_path) {
		absolute_source_path = realpath(source_path, NULL);

		if (!search_path) {
			// Use source to infer search path
			source_copy = realpath(source_path, NULL);

			absolute_search_path = mmd_dirname(source_copy);
		}
	}

	if (search_path) {
		absolute_search_path = realpath(search_path, NULL);
	}

	if (absolute_search_path) {
		stack * s = stack_new(0);

		read_ctx * r = mmd_transclude_recursive(buffer, options, absolute_search_path, absolute_source_path, s);

		read_ctx_free(r);
		stack_free(s);
	}

	free(absolute_search_path);
	free(absolute_source_path);
	free(source_copy);
}

