/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file write_ctx.h

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


#ifndef WRITE_CTX_LIBMULTIMARKDOWN7_H
#define WRITE_CTX_LIBMULTIMARKDOWN7_H

#include <stdbool.h>


/// Context information for parsing process
struct write_ctx {
	short			padding;
	short			skip_blocks;

	bool			in_recursive;
	bool			in_definition_list;
	bool			in_endnote;
	bool 			skip_endnote_label;
	bool 			in_figure;

	char			in_table_header;
	unsigned char	table_col_count;
	unsigned char	table_cell_count;
	char			table_alignment[255];

	char 	*		endnote_return;
	int				endnote_return_para_counter;

	uint16_t		random_footnote_seed;
	int				header_count;

	stack 	*		used_cite_stack;
	stack 	*		used_glos_stack;
	stack 	*		used_note_stack;

	stack *			inline_endnotes_to_free;
};

typedef struct write_ctx write_ctx;


write_ctx * write_ctx_new(void);
void write_ctx_free(write_ctx * c);

void pad(text_buffer * out, short n, write_ctx * c);

bool write_ctx_table_has_caption(mmd_node * t);
void write_ctx_get_table_alignments(write_ctx * c, mmd_node * table, const char * text);

#endif
