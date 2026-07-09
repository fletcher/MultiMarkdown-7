/**

	libMultiMarkdown7 -- C parser for Markdown with additional features and multiple output formats.

	@file toc_node.c

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
#include "mmd_node.h"
#include "toc_node.h"

#include "mmd_utilities.h"

#include "vector_line_node.h"
#include "read_ctx.h"
#include "mmd_parser_hand_2.h"
#include "write_ctx.h"

#include "export_core.h"

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


toc_node * mmd_toc_filename(const char * fname, uint32_t options) {
	toc_node * t = NULL;
	FILE * in = flex_fopen(fname);

	if (in) {
		t = mmd_toc_file(in, options);
		fclose(in);
	}

	return t;
}


toc_node * mmd_toc_file(FILE * in, uint32_t options) {
	text_buffer * buffer = buffer_file(in, kDEFAULTCAPACITY);

	toc_node * t = mmd_toc_buffer(buffer, options);

	text_buffer_free(buffer, 1);

	return t;
}


toc_node * mmd_toc_str(const char * text, uint32_t options) {
	size_t len = strlen(text);

	return mmd_toc_str_len(text, len, options);
}


toc_node * mmd_toc_str_len(const char * text, size_t in_len, uint32_t options) {
	// We need to ensure that the text is null-terminated
	text_buffer * buffer = text_buffer_new(in_len + 1);
	text_buffer_append_text(buffer, text, in_len);

	toc_node * t = mmd_toc_buffer(buffer, options);

	text_buffer_free(buffer, 1);

	return t;
}


toc_node * mmd_toc_buffer(text_buffer * buffer, uint32_t options) {
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

	toc_node * t = read_ctx_get_toc(r, buffer->text, buffer->len);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	clock_gettime(CLOCK_MONOTONIC_RAW, &mid);
#endif

	vector_line_node_free(vl);
	mmd_node_pool_free(vn);
	read_ctx_free(r);

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

	return t;
}


static int header_is_valid(header * h) {
	char * peek = (char *) h->text;

	F(i, 3) {
		if (*peek == ' ') {
			peek++;
		}
	}

	switch (*peek) {
		case '\t':
		case '>':
		case ' ':
			return 0;
	}

	return 1;
}


static toc_node * extract_toc_entry(size_t len, size_t * counter, int level, read_ctx * r) {
	toc_node * t = NULL;
	toc_node * w = NULL;

	header * h, * next;
	int h_level, next_level;

	while (*counter < r->header_stack->size) {
		h = stack_peek_index(r->header_stack, *counter);
		h_level = raw_level_for_header(h->node);

		if (!header_is_valid(h)) {
			(*counter)++;
			continue;
		}

		if (h_level >= level) {
			// This header is a direct descendant of the parent
			if (w) {
				w->next = calloc(1, sizeof(toc_node));
				w = w->next;
			} else {
				t = calloc(1, sizeof(toc_node));
				w = t;
			}

			if (w) {
				w->start = h->node->start;
				w->label = my_strdup(h->key);

				// mmd_node_tree_describe(h->node->content, stdout, NULL, 0);

				text_buffer * temp = text_buffer_new(0);
				export_plain_text(h->node->content, h->text, temp);

				// fwrite(temp->text, temp->len, 1, stdout);

				text_buffer_trim_trailing_newline(temp);
				text_buffer_trim_trailing_whitespace(temp);

				w->title = my_strndup(temp->text, temp->len);

				text_buffer_free(temp, 1);
			} else {
				// Error
				return NULL;
			}

loop:

			if (*counter < r->header_stack->size - 1) {
				next = stack_peek_index(r->header_stack, *counter + 1);
				next_level = raw_level_for_header(next->node);

				if (!header_is_valid(next)) {
					(*counter)++;
					goto loop;
				}

				w->end = next->text - h->text + w->start;

				if (next_level > h_level) {
					// This entry has children
					(*counter)++;
					w->child = extract_toc_entry(len, counter, h_level + 1, r);
				}
			} else {
				// This is the last entry in the document
				w->end = len;
			}
		} else if (h_level < level) {
			// Decrement counter anx exit this level
			(*counter)--;
			break;
		}

		// Increment counter
		(*counter)++;
	}

	return t;
}


toc_node * read_ctx_get_toc(read_ctx * r, const char * text, size_t len) {
	size_t counter = 0;

	if (text && len) {}

	return extract_toc_entry(len, &counter, 0, r);
}

void toc_node_free(toc_node * t) {
	if (t) {
		free(t->title);
		free(t->label);

		if (t->child) {
			toc_node_tree_free(t->child);
		}

		free(t);
	}
}


void toc_node_tree_free(toc_node * t) {
	toc_node * next;

	while (t) {
		next = t->next;
		toc_node_free(t);
		t = next;
	}
}
