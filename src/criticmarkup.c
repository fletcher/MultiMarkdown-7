/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file criticmarkup.c

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

#include "read_ctx.h"
#include "mmd_node.h"
#include "mmd_span_parser.h"
#include "mmd_scanner.h"
#include "text_buffer.h"

#include "criticmarkup.h"


#ifdef TEST
	#include "CuTest.h"
#endif


/// Scan text for CriticMarkup tokens
int criticmarkup_token_scan(Scanner * s);


static PairRule cm_pairings[OBJECT_REPLACEMENT_CHARACTER] = {
	[TOKEN_CM_ADD_CLOSE] =	{ 0, TOKEN_CM_ADD_OPEN,	TOKEN_PAIR_CM_ADD	},
	[TOKEN_CM_DEL_CLOSE] =	{ 0, TOKEN_CM_DEL_OPEN,	TOKEN_PAIR_CM_DEL	},
	[TOKEN_CM_COM_CLOSE] =	{ 0, TOKEN_CM_COM_OPEN,	TOKEN_PAIR_CM_COM	},
	[TOKEN_CM_HI_CLOSE] =	{ 0, TOKEN_CM_HI_OPEN,	TOKEN_PAIR_CM_HI	},
	[TOKEN_CM_SUB_DIV] =	{ 0, TOKEN_CM_SUB_OPEN,	TOKEN_PAIR_CM_SUB_DEL	},
	[TOKEN_CM_SUB_CLOSE] =	{ 0, TOKEN_CM_SUB_DIV,	TOKEN_PAIR_CM_SUB_ADD	},
};


mmd_node * criticmarkup_tokenize(mmd_node_pool * p, const char * text, size_t len) {
	Scanner s = mmd_scanner(text, len);

	mmd_node * root = NULL;
	mmd_node * last = NULL;

	// Tokenize the text, but only for CriticMarkup tokens
	do {
		s.curType = criticmarkup_token_scan(&s);

		mmd_node * n = mmd_node_new(p, s.curType, s.start - s.text, s.cur - s.start);

		if (last) {
			last->next = n;
			last = n;
		} else {
			root = n;
			last = n;
		}
	} while (s.curType != TOKEN_EOF);

	// Pair the CriticMarkup tokens
	read_ctx * c = read_ctx_new(0);
	analyze_token_chain(p, root, cm_pairings, text, c, 0);
	read_ctx_free(c);

	return root;
}


static void accept_token_tree(text_buffer * buffer, mmd_node * n, size_t * delta) {
	while (n) {
		switch (n->type) {
			case TOKEN_PAIR_CM_ADD:
			case TOKEN_PAIR_CM_SUB_ADD:
				// Erase opener
				text_buffer_delete_range(buffer, n->start - *delta, n->len);
				*delta += n->len;

				// Descent
				accept_token_tree(buffer, n->child, delta);

				// Erase closer
				n = n->next;
				text_buffer_delete_range(buffer, n->start - *delta, n->len);
				*delta += n->len;

				break;

			case TOKEN_PAIR_CM_DEL: {
				// Erase all
				size_t len = n->next->start + n->next->len - n->start;
				text_buffer_delete_range(buffer, n->start - *delta, len);
				*delta += len;
			}

			break;

			case TOKEN_PAIR_CM_COM: {
				// Erase all
				size_t len = n->next->start + n->next->len - n->start;
				text_buffer_delete_range(buffer, n->start - *delta, len);
				*delta += len;
				n = n->next;
			}

			break;

			case TOKEN_PAIR_CM_SUB_DEL: {
				// Erase all
				size_t len = n->next->start - n->start;
				text_buffer_delete_range(buffer, n->start - *delta, len);
				*delta += len;
			}

			break;

			case TOKEN_PAIR_CM_HI:
				// Erase opener
				text_buffer_delete_range(buffer, n->start - *delta, n->len);
				*delta += n->len;

				// Descent
				accept_token_tree(buffer, n->child, delta);

				// Erase closer
				n = n->next;
				text_buffer_delete_range(buffer, n->start - *delta, n->len);
				*delta += n->len;

				break;
		}

		n = n->next;
	}
}


static void reject_token_tree(text_buffer * buffer, mmd_node * n, size_t * delta) {
	while (n) {
		switch (n->type) {
			case TOKEN_PAIR_CM_DEL:
				// Erase opener
				text_buffer_delete_range(buffer, n->start - *delta, n->len);
				*delta += n->len;

				// Descent
				accept_token_tree(buffer, n->child, delta);

				// Erase closer
				n = n->next;
				text_buffer_delete_range(buffer, n->start - *delta, n->len);
				*delta += n->len;

				break;

			case TOKEN_PAIR_CM_SUB_DEL:
				// Erase opener
				text_buffer_delete_range(buffer, n->start - *delta, n->len);
				*delta += n->len;

				// Descent
				accept_token_tree(buffer, n->child, delta);

				break;

			case TOKEN_PAIR_CM_ADD:
			case TOKEN_PAIR_CM_SUB_ADD:
			case TOKEN_PAIR_CM_COM: {
				// Erase all
				size_t len = n->next->start + n->next->len - n->start;
				text_buffer_delete_range(buffer, n->start - *delta, len);
				*delta += len;
				n = n->next;
			}

			break;

			case TOKEN_PAIR_CM_HI:
				// Erase opener
				text_buffer_delete_range(buffer, n->start - *delta, n->len);
				*delta += n->len;

				// Descent
				accept_token_tree(buffer, n->child, delta);

				// Erase closer
				n = n->next;
				text_buffer_delete_range(buffer, n->start - *delta, n->len);
				*delta += n->len;

				break;
		}

		n = n->next;
	}
}


void criticmarkup_accept(text_buffer * buffer) {
	mmd_node_pool * p = mmd_node_pool_new(0);

	mmd_node * n = criticmarkup_tokenize(p, buffer->text, buffer->len);

	size_t delta = 0;
	accept_token_tree(buffer, n, &delta);

	mmd_node_pool_free(p);
}

void criticmarkup_reject(text_buffer * buffer) {
	mmd_node_pool * p = mmd_node_pool_new(0);

	mmd_node * n = criticmarkup_tokenize(p, buffer->text, buffer->len);

	size_t delta = 0;
	reject_token_tree(buffer, n, &delta);

	mmd_node_pool_free(p);
}

