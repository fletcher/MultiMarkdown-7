/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file opml.c

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

#include "mmd_node.h"
#include "text_buffer.h"
#include "read_ctx.h"
#include "write_ctx.h"
#include "mmd_span_parser.h"
#include "mmd_scanner.h"
#include "mmd_token_scanner.h"
#include "mmd_utilities.h"

#include "export_core.h"
#include "opml.h"


#ifdef TEST
	#include "CuTest.h"
#endif

#define F(i,n) for(int i= 0;i<n;i++)


static void export_opml_raw_text(const char * text, size_t len, text_buffer * out) {
	const char * stop = text + len;

	while (text < stop) {
		switch (*text) {
			case '&':
				mmd_print_const(out, "&amp;");
				break;

			case '<':
				mmd_print_const(out, "&lt;");
				break;

			case '>':
				mmd_print_const(out, "&gt;");
				break;

			case '"':
				mmd_print_const(out, "&quot;");
				break;

			case '\'':
				mmd_print_const(out, "&apos;");
				break;

			case '\n':
				mmd_print_const(out, "&#10;");
				break;

			case '\r':
				mmd_print_const(out, "&#13;");
				break;

			case '\t':
				mmd_print_const(out, "&#9;");
				break;

			default:
				text_buffer_append_c(out, *text);
				break;
		}

		text++;
	}
}


static void export_opml_header(text_buffer * out, read_ctx * r) {
	mmd_print_const(out, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<opml version=\"1.0\">\n");

	meta * m = read_ctx_get_meta(r, "title");

	if (m) {
		mmd_print_const(out, "\t<head><title>");

		export_opml_raw_text(m->value, m->value_len, out);

		mmd_print_const(out, "</title></head>\n");
	}

	mmd_print_const(out, "\t<body>\n");
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
			return 0;
	}

	return 1;
}


static void export_opml_preamble(text_buffer * out, mmd_node * b, const char * text, size_t len, read_ctx * r) {
	size_t pre_start = 0;
	size_t pre_len = len;

	size_t counter = 0;

	while (counter < r->header_stack->size) {
		header * h = stack_peek_index(r->header_stack, 0);

		if (header_is_valid(h)) {
			if (h->text == text) {
				pre_len = 0;
			} else {
				pre_len = h->text - text;
			}

			break;
		} else {
			counter++;
		}
	}

	if (pre_len) {
		if (b && b->type == BLOCK_META) {
			pre_start = b->start + b->len;
			pre_len -= pre_start;
		} else if (b && b->start) {
			pre_start = b->start;
			pre_len -= b->start;
		}

		mmd_print_const(out, "\t\t<outline text=\"&gt;&gt;Preamble&lt;&lt;\" _note=\"");

		export_opml_raw_text(&text[pre_start], pre_len, out);

		mmd_print_const(out, "\"/>\n");
	}
}


static void export_opml_metadata(text_buffer * out, read_ctx * r) {
	meta * m, * m_tmp;

	if (r->meta_hash) {
		mmd_print_const(out, "\t\t<outline text=\"&gt;&gt;Metadata&lt;&lt;\">\n");

		HASH_ITER(hh, r->meta_hash, m, m_tmp) {
			mmd_print_const(out, "\t\t\t<outline text=\"");
			export_opml_raw_text(m->key, strlen(m->key), out);
			mmd_print_const(out, "\" _note=\"");
			export_opml_raw_text(m->value, m->value_len, out);
			mmd_print_const(out, "\"/>\n");
		}

		mmd_print_const(out, "\t\t</outline>\n");
	}
}


static void export_opml_footer(text_buffer * out) {
	mmd_print_const(out, "\t</body>\n</opml>\n");
}


static void export_opml_outline(text_buffer * out, const char * text, size_t len, size_t * counter, int level, int depth, read_ctx * r, write_ctx * w, uint32_t options) {
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
			F(i, (depth + 2)) {
				text_buffer_append_c(out, '\t');
			}

			mmd_print_const(out, "<outline text=\"");

			export_opml_raw_text(&h->text[h->c_start], h->c_len, out);

loop:

			if (*counter < r->header_stack->size - 1) {
				next = stack_peek_index(r->header_stack, *counter + 1);
				next_level = raw_level_for_header(next->node);

				if (!header_is_valid(next)) {
					(*counter)++;
					goto loop;
				}

				// Everything until next header belongs here
				mmd_print_const(out, "\" _note=\"");
				export_opml_raw_text(&h->text[h->text_len], next->text - h->text - h->text_len, out);

				if (next_level > h_level) {
					// This entry has children
					mmd_print_const(out, "\">\n");

					(*counter)++;
					export_opml_outline(out, text, len, counter, h_level + 1, depth + 1, r, w, options);

					F(i, (depth + 2)) {
						text_buffer_append_c(out, '\t');
					}
					mmd_print_const(out, "</outline>\n");
				} else {
					// This entry has no children
					mmd_print_const(out, "\"/>\n");
				}
			} else {
				// This is the last entry in the document

				// Everything until the end of the document belongs here
				mmd_print_const(out, "\" _note=\"");
				export_opml_raw_text(&h->text[h->text_len], &text[len] - h->text - h->text_len, out);

				mmd_print_const(out, "\"/>\n");
			}
		} else if (h_level < level) {
			// Decrement counter and exit this level
			(*counter)--;
			break;
		}

		// Increment counter
		(*counter)++;
	}
}


void export_opml(mmd_node * b, const char * text, size_t len, text_buffer * out, read_ctx * r, uint32_t options) {
	write_ctx * w = write_ctx_new();

	export_opml_header(out, r);

	export_opml_preamble(out, b, text, len, r);

	size_t counter = 0;

	export_opml_outline(out, text, len, &counter, 0, 0, r, w, options);

	export_opml_metadata(out, r);

	export_opml_footer(out);

	pad(out, 1, w);
	write_ctx_free(w);
}
