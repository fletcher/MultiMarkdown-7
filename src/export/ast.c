/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file ast.c

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
#include "ast.h"

#define F(i,n) for(int i= 0;i<n;i++)


static void ast_node_tree(mmd_node * n, const char * text, unsigned short depth, char marker, size_t offset, text_buffer * out);


static void ast_node(mmd_node * n, const char * text, unsigned short depth, char marker, size_t offset, text_buffer * out) {
	if (n) {
		F(i, depth) {
			text_buffer_append_c(out, '\t');
		}

		if (text) {
			text_buffer_append_printf(out, "%c (%d) %zu:%zu\t'%.*s'\n", marker, n->type, n->start, n->len, (int)n->len, &text[n->start + offset]);
		} else {
			text_buffer_append_printf(out, "%c (%d) %zu:%zu\n", marker, n->type, n->start, n->len);
		}

		if (n->child) {
			if (MMD_NODE_IS_BLOCK(n)) {
				ast_node_tree(n->child, text, depth + 1, '*', offset + n->start, out);
			} else {
				ast_node_tree(n->child, text, depth + 1, '+', offset, out);
			}
		}

		if (n->content) {
			ast_node_tree(n->content, text, depth + 1, '-', offset + n->start, out);
		}
	}
}


static void ast_node_tree(mmd_node * n, const char * text, unsigned short depth, char marker, size_t offset, text_buffer * out) {
	while (n) {
		ast_node(n, text, depth, marker, offset, out);

		n = n->next;
	}
}


static void hash_node_tree(mmd_node * n, unsigned short depth, char marker, size_t offset, text_buffer * out);


static void hash_node(mmd_node * n, unsigned short depth, char marker, size_t offset, text_buffer * out) {
	if (n) {
		F(i, depth) {
			text_buffer_append_c(out, '\t');
		}

		text_buffer_append_printf(out, "%c (%d) %zu:%zu - %u\n", marker, n->type, n->start, n->len, n->hash);

		if (n->child) {
			if (MMD_NODE_IS_BLOCK(n)) {
				hash_node_tree(n->child, depth + 1, '*', offset + n->start, out);
			} else {
				hash_node_tree(n->child, depth + 1, '+', offset, out);
			}
		}

		if (n->content) {
			hash_node_tree(n->content, depth + 1, '-', offset + n->start, out);
		}
	}
}


static void hash_node_tree(mmd_node * n, unsigned short depth, char marker, size_t offset, text_buffer * out) {
	while (n) {
		hash_node(n, depth, marker, offset, out);

		n = n->next;
	}
}


void export_ast(mmd_node * b, const char * text, text_buffer * out) {
	ast_node_tree(b, text, 0, '*', 0, out);
}


void export_hash(mmd_node * b, text_buffer * out) {
	hash_node_tree(b, 0, '*', 0, out);
}
