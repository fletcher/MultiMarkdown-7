/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file outline.c

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
#include "outline.h"
#include "libMultiMarkdown7.h"
#include "zip.h"


#ifdef TEST
	#include "CuTest.h"
#endif

#define F(i,n) for(int i= 0;i<n;i++)


static void export_outline_raw_text(const char * text, size_t len, text_buffer * out) {
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
				if (text[1] != '\n') {
					mmd_print_const(out, "&#13;");
				}

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


static void note(text_buffer * out, enum output_format format) {
	switch (format) {
		case FORMAT_OPML:
			mmd_print_const(out, "\" _note=\"");
			break;

		case FORMAT_ITMZ:
			mmd_print_const(out, "\" note=\"");
			break;

		default:
			break;
	}
}


static void export_uuid(text_buffer * out) {
	char * uuid = uuid_new();
	text_buffer_append_printf(out, "uuid=\"%s\" ", uuid);
	free(uuid);
}


static void item_open(text_buffer * out, enum output_format format) {
	switch (format) {
		case FORMAT_OPML:
			mmd_print_const(out, "<outline text=\"");
			break;

		case FORMAT_ITMZ:
			mmd_print_const(out, "<topic ");
			export_uuid(out);
			mmd_print_const(out, "text=\"");
			break;

		default:
			break;
	}
}


static void item_close(text_buffer * out, enum output_format format) {
	switch (format) {
		case FORMAT_OPML:
			mmd_print_const(out, "</outline>\n");
			break;

		case FORMAT_ITMZ:
			mmd_print_const(out, "</topic>\n");
			break;

		default:
			break;
	}
}


static void start(text_buffer * out, enum output_format format) {
	switch (format) {
		case FORMAT_OPML:
			mmd_print_const(out, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<opml version=\"1.0\">\n");
			break;

		case FORMAT_ITMZ:
			mmd_print_const(out, "<iThoughts>\n");
			break;

		default:
			break;
	}
}


static void title(text_buffer * out, meta * m, enum output_format format) {
	switch (format) {
		case FORMAT_OPML:
			if (m) {
				mmd_print_const(out, "\t<head><title>");
				export_outline_raw_text(m->value, m->value_len, out);
				mmd_print_const(out, "</title></head>\n");
			}

			mmd_print_const(out, "\t<body>\n");
			break;

		case FORMAT_ITMZ:
			if (m) {
				mmd_print_const(out, "\t<topic ");
				export_uuid(out);
				mmd_print_const(out, "text=\"");

				export_outline_raw_text(m->value, m->value_len, out);

				mmd_print_const(out, "\">\n");
			} else {
				mmd_print_const(out, "\t<topic ");
				export_uuid(out);
				mmd_print_const(out, "text=\"\">\n");
			}

			break;

		default:
			break;
	}
}


static void export_outline_header(text_buffer * out, read_ctx * r, enum output_format format) {
	start(out, format);

	meta * m = read_ctx_get_meta(r, "title");
	title(out, m, format);
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


static void export_outline_preamble(text_buffer * out, mmd_node * b, const char * text, size_t len, read_ctx * r, enum output_format format) {
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

		switch (format) {
			case FORMAT_OPML:
				mmd_print_const(out, "\t\t<outline text=\"&gt;&gt;Preamble&lt;&lt;\" _note=\"");
				export_outline_raw_text(&text[pre_start], pre_len, out);
				mmd_print_const(out, "\"/>\n");
				break;

			case FORMAT_ITMZ:
				mmd_print_const(out, "\t\t<topic ");
				export_uuid(out);
				mmd_print_const(out, "text=\"&gt;&gt;Preamble&lt;&lt;\" note=\"");
				export_outline_raw_text(&text[pre_start], pre_len, out);
				mmd_print_const(out, "\"/>\n");
				break;

			default:
				break;
		}
	}
}


static void export_outline_metadata(text_buffer * out, read_ctx * r, enum output_format format) {
	meta * m, * m_tmp;

	if (r->meta_hash) {
		switch (format) {
			case FORMAT_OPML:
				mmd_print_const(out, "\t\t<outline text=\"&gt;&gt;Metadata&lt;&lt;\">\n");

				HASH_ITER(hh, r->meta_hash, m, m_tmp) {
					mmd_print_const(out, "\t\t\t<outline text=\"");
					export_outline_raw_text(m->key, strlen(m->key), out);
					mmd_print_const(out, "\" _note=\"");
					export_outline_raw_text(m->value, m->value_len, out);
					mmd_print_const(out, "\"/>\n");
				}

				mmd_print_const(out, "\t\t</outline>\n");
				break;

			case FORMAT_ITMZ:
				mmd_print_const(out, "\t\t<topic ");
				export_uuid(out);
				mmd_print_const(out, "text=\"&gt;&gt;Metadata&lt;&lt;\">\n");

				HASH_ITER(hh, r->meta_hash, m, m_tmp) {
					mmd_print_const(out, "\t\t\t<topic ");
					export_uuid(out);
					mmd_print_const(out, "text=\"");
					export_outline_raw_text(m->key, strlen(m->key), out);
					mmd_print_const(out, "\" note=\"");
					export_outline_raw_text(m->value, m->value_len, out);
					mmd_print_const(out, "\"/>\n");
				}

				mmd_print_const(out, "\t\t</topic>\n");
				break;

			default:
				break;
		}
	}
}


static void export_outline_footer(text_buffer * out, enum output_format format) {
	switch (format) {
		case FORMAT_OPML:
			mmd_print_const(out, "\t</body>\n</opml>\n");
			break;

		case FORMAT_ITMZ:
			mmd_print_const(out, "\t</topic>\n</iThoughts>\n");
			break;

		default:
			break;
	}
}


static void export_outline_outline(text_buffer * out, const char * text, size_t len, size_t * counter, int level, int depth, read_ctx * r, write_ctx * w, uint32_t options, enum output_format format) {
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

			item_open(out, format);

			export_outline_raw_text(&h->text[h->c_start], h->c_len, out);

loop:

			if (*counter < r->header_stack->size - 1) {
				next = stack_peek_index(r->header_stack, *counter + 1);
				next_level = raw_level_for_header(next->node);

				if (!header_is_valid(next)) {
					(*counter)++;
					goto loop;
				}

				// Everything until next header belongs here
				note(out, format);
				export_outline_raw_text(&h->text[h->text_len], next->text - h->text - h->text_len, out);

				if (next_level > h_level) {
					// This entry has children
					mmd_print_const(out, "\">\n");

					(*counter)++;
					export_outline_outline(out, text, len, counter, h_level + 1, depth + 1, r, w, options, format);

					F(i, (depth + 2)) {
						text_buffer_append_c(out, '\t');
					}
					item_close(out, format);
				} else {
					// This entry has no children
					mmd_print_const(out, "\"/>\n");
				}
			} else {
				// This is the last entry in the document

				// Everything until the end of the document belongs here
				note(out, format);
				export_outline_raw_text(&h->text[h->text_len], &text[len] - h->text - h->text_len, out);
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


void export_outline(mmd_node * b, const char * text, size_t len, text_buffer * out, read_ctx * r, uint32_t options, enum output_format format) {
	write_ctx * w = write_ctx_new();

	export_outline_header(out, r, format);

	export_outline_preamble(out, b, text, len, r, format);

	size_t counter = 0;

	export_outline_outline(out, text, len, &counter, 0, 0, r, w, options, format);

	export_outline_metadata(out, r, format);

	export_outline_footer(out, format);

	pad(out, 1, w);
	write_ctx_free(w);

	if (format == FORMAT_ITMZ) {
		// Create zip archive
		mz_zip_archive zip;
		mz_bool status = zip_new_archive(&zip);

		if (!mz_zip_writer_add_mem(&zip, "mapdata.xml", out->text, out->len, MZ_BEST_COMPRESSION)) {
			fprintf(stderr, "Error adding main content to itmz zip archive.\n");
		}


		// Finalize zip archive and insert in out text_buffer
		free(out->text);
		out->text = NULL;
		status = mz_zip_writer_finalize_heap_archive(&zip, (void **) & (out->text), (size_t *) & (out->len));

		if (!status) {
			fprintf(stderr, "Error finalizing zip archive.\n");
			free(out->text);
			out->text = malloc(out->capacity + 1);
			out->len = 0;
		} else {
			out->capacity = out->len;
		}

		mz_zip_writer_end(&zip);
	}
}
