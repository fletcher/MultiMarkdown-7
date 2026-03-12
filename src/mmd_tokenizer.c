/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_tokenizer.c

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

#include "char.h"
#include "mmd_node.h"
#include "read_ctx.h"
#include "mmd_scanner.h"
#include "mmd_token_scanner.h"

#include "mmd_tokenizer.h"


#ifdef TEST
	#include "CuTest.h"
#endif


struct mmd_tokenizer {
	read_ctx 	*	ctx;

	const char *	text;		//!< Pointer to text for entire block
	const char *	stop;		//!< Pointer to end of text for entire block

	const char *	c_start;	//!< Pointer to start of line content
	const char *	c_end;	//!< Pointer to end of line content
	mmd_line_node *	line;		//!< Current line being tokenized


	Scanner			s;			//!< Scanner

	mmd_node 	*	t;			//!< Next token
};


static void mmd_tokenizer_prep_for_line(mmd_tokenizer * z, mmd_line_node * l) {
	if (z) {
		z->line = l;

		if (l) {
			z->c_start = &z->text[l->general.start + l->c_start];
			z->c_end = z->c_start + l->c_len;

			// Skip leading whitespace
			while (char_is_whitespace(*(z->c_start))) {
				z->c_start++;
			}

			// Advance scanner to start of line
			z->s.cur = &z->text[l->general.start];

			// Skip leading whitespace
			while (char_is_whitespace(*(z->s.cur))) {
				z->s.cur++;
			}
		} else {
			z->s.cur = z->s.stop;
		}
	}
}


static mmd_node * mmd_tokenizer_next_token(mmd_tokenizer * z, mmd_node_pool * p, uint32_t options) {
	Scanner * s = &z->s;

	if (s->cur < s->stop) {
		// Remember starting point
		const char * start = s->cur;

		// Scan next token
		s->start = start;
		s->curType = mmd_token_scan(s, options);

		if (s->cur <= z->c_start) {
			// We're still in the prefix
			switch (s->curType) {
				case TOKEN_ANGLE_RIGHT:
					s->curType = TOKEN_BLOCKQUOTE_MARKER;
					break;

				case TOKEN_COLON:
					if (z->ctx->is_definition || z->ctx->is_endnote) {
						s->curType = TOKEN_DEFLIST_COLON;
					}

					break;

				case TOKEN_HASH:
					s->curType = TOKEN_ATX_MARKER;
					break;

				case TOKEN_STAR:
				case TOKEN_PLUS:
				case TOKEN_MINUS:
				case TEXT_NUMBER_POSS_LIST:
					s->curType = TOKEN_LIST_MARKER;
					break;
			}
		} else if (start < z->c_start) {
			// Token started in prefix
		} else if (s->cur > z->c_end) {
			// Token extends after the line content (e.g. closing '#' in ATX header)
			switch (s->curType) {
				case TOKEN_HASH:
					s->curType = TOKEN_ATX_MARKER;
					break;
			}
		}

		mmd_node * t = NULL;

		if ((s->c_start == start) && (s->cur > s->start)) {
			// Single token
			if (s->curType == TOKEN_EOF) {
				return NULL;
			}

			t = mmd_node_new(p, s->curType, s->start - s->text, s->cur - s->start);
		} else {
			// Plain text before the actual token, so return both
			if (s->curType == TOKEN_EOF) {
				// Build text token until end of input
				t = mmd_node_new(p, TOKEN_TEXT, start - s->text, s->stop - start);

				t->next = NULL;
			} else {
				// Build first token (TOKEN_TEXT)
				t = mmd_node_new(p, TOKEN_TEXT, start - s->text, s->c_start - start);

				// Trim trailing whitespace from plain text in certain cases
				switch (s->curType) {
					case TOKEN_ATX_MARKER:
						while (t->len && char_is_whitespace(s->text[t->start + t->len - 1])) {
							t->len--;
						}

						break;
				}

				// Append next token
				t->next = mmd_node_new(p, s->curType, s->c_start - s->text, s->cur - s->c_start);
			}
		}

		// If we're in the prefix, advance past whitespace
		if (s->cur < z->c_start) {
			while (s->cur < z->stop && char_is_whitespace(*(s->cur))) {
				s->cur++;
			}
		}

		// If we found a line break, advance line pointer
		if ((s->curType == TOKEN_NL) || (s->curType == TOKEN_LINEBREAK)) {
			if (z->line) {
				mmd_tokenizer_prep_for_line(z, (mmd_line_node *) z->line->general.next);

				while (s->cur < z->stop && char_is_whitespace(*(s->cur))) {
					s->cur++;
				}
			}
		}

		// Advance past whitespace after token
		// TODO: can do this always for a tokenizer for syntax highlighting
		// while (char_is_whitespace(*(s->cur))) {
		// 	s->cur++;
		// }

		return t;
	} else {
		return NULL;
	}
}


/// Create new tokenizer for a given block
mmd_tokenizer * mmd_tokenizer_new(const char * text, mmd_node * block, read_ctx * c, mmd_node_pool * p, uint32_t options) {
	mmd_tokenizer * z = calloc(1, sizeof(mmd_tokenizer));

	if (z) {
		z->ctx = c;

		z->text = text;
		z->stop = text + block->len;

		switch (block->type) {
			case BLOCK_SETEXT_1:
			case BLOCK_SETEXT_2: {
				// Don't tokenize last line
				mmd_node * l = block->child;

				while (l->next) {
					l = l->next;
				}

				z->s = mmd_scanner(text, l->start);
			}
			break;

			default:
				z->s = mmd_scanner(text, block->len);
				break;
		}

		if (MMD_NODE_IS_BLOCK(block)) {
			mmd_tokenizer_prep_for_line(z, (mmd_line_node *) block->child);
		}

		z->t = mmd_tokenizer_next_token(z, p, options);
	}

	return z;
}


void mmd_tokenizer_free(mmd_tokenizer * z) {
	if (z) {
		if (z->t) {
			free(z->t);
		}

		free(z);
	}
}


/// Type for next token from tokenizer
unsigned char mmd_tokenizer_node_type(mmd_tokenizer * z) {
	if (z && z->t) {
		return z->t->type;
	} else {
		return '\0';
	}
}


/// Get current token and scan the next
mmd_node * mmd_tokenizer_accept_token(mmd_tokenizer * z, mmd_node_pool * p, uint32_t options) {
	mmd_node * r = z->t;

	if (r == NULL) {
		// No more tokens
	} else if (r->next) {
		// Two tokens were scanned -- load the next one
		z->t = r->next;
		r->next = NULL;
	} else {
		// Load the next token
		z->t = mmd_tokenizer_next_token(z, p, options);
	}

	// TODO: Debugging only; remove for production
	if (z->t) {
		if (r->start > z->t->start) {
			fprintf(stderr, "\n***\nmmd_tokenizer_accept_token error\n");
		}
	}

	return r;
}

