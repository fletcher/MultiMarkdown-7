/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file write_ctx.c

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

#include "text_buffer.h"
#include "libMultiMarkdown7.h"
#include "stack.h"
#include "read_ctx.h"
#include "mmd_scanner.h"
#include "mmd_token_scanner.h"
#include "write_ctx.h"


write_ctx * write_ctx_new(void) {
	write_ctx * c = calloc(1, sizeof(write_ctx));

	if (c) {
		c->padding = 2;

		c->used_cite_stack = stack_new(8);
		c->used_glos_stack = stack_new(8);
		c->used_note_stack = stack_new(8);

		c->inline_endnotes_to_free = stack_new(8);

		c->random_footnote_seed = (uint16_t) rand();
	}

	return c;
}


void write_ctx_free(write_ctx * c) {
	if (c) {
		stack_free(c->used_cite_stack);
		stack_free(c->used_glos_stack);
		stack_free(c->used_note_stack);

		endnote_def * e;

		while ((e = stack_pop(c->inline_endnotes_to_free))) {
			endnote_def_free(e);
		}

		stack_free(c->inline_endnotes_to_free);

		free(c);
	}
}


void pad(text_buffer * out, short n, write_ctx * c) {
	while (n > c->padding) {
		text_buffer_append_c(out, '\n');
		c->padding++;
	}
}


void write_ctx_get_table_alignments(write_ctx * c, mmd_node * t, const char * text) {
	mmd_node * walker = t->child->child;

	c->table_col_count = 0;
	c->table_alignment[0] = 0;

	if (walker == NULL) {
		return;
	}

	// Find separator line
	while (walker && walker->type != BLOCK_TABLE_SEPARATOR) {
		walker = walker->next;
	}

	if (walker == NULL) {
		return;
	}

	text = &text[walker->start];
	walker = walker->content;

	unsigned char alignment = 0;

	while (walker) {
		switch (walker->type) {
			case TOKEN_TABLE_CELL:
				alignment = scan_table_alignment(&text[walker->start]);

				switch (alignment) {
					case ALIGN_LEFT:
						c->table_alignment[c->table_col_count++] = 'l';
						break;

					case ALIGN_RIGHT:
						c->table_alignment[c->table_col_count++] = 'r';
						break;

					case ALIGN_CENTER:
						c->table_alignment[c->table_col_count++] = 'c';
						break;

					case ALIGN_LEFT | ALIGN_WRAP:
						c->table_alignment[c->table_col_count++] = 'L';
						break;

					case ALIGN_RIGHT | ALIGN_WRAP:
						c->table_alignment[c->table_col_count++] = 'R';
						break;

					case ALIGN_CENTER | ALIGN_WRAP:
						c->table_alignment[c->table_col_count++] = 'C';
						break;

					case ALIGN_WRAP:
						c->table_alignment[c->table_col_count++] = 'N';
						break;

					default:
						c->table_alignment[c->table_col_count++] = 'n';
						break;
				}

				break;
		}

		walker = walker->next;
	}
}

