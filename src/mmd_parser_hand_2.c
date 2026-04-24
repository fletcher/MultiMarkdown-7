/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_parser_hand_2.c

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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libMultiMarkdown7.h"

#include "vector_line_node.h"
#include "mmd_node_pool.h"
#include "read_ctx.h"

#include "text_buffer.h"
#include "mmd_core.h"
#include "mmd_scanner.h"
#include "mmd_line_scanner.h"
#include "mmd_token_scanner.h"
#include "mmd_node.h"
#include "mmd_utilities.h"

#include "mmd_parser_hand_2.h"
#include "mmd_span_parser.h"

#include "aho-corasick.h"
#include "char.h"
#include "vector_line_node.h"

#define F(i,n) for(int i= 0;i<n;i++)

static mmd_node * recursive_indent_parse(mmd_line_node * l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options);
static mmd_node * recursive_blockquote_parse(mmd_node * l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options);
static mmd_node * recursive_endnote_parse(endnote_def ** e, mmd_node * l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options);
static mmd_node * mask_manual_label_token(mmd_node * b);


static mmd_line_node * scanner_next_line(Scanner * s, uint32_t options) {
	if (s->cur < s->stop) {
		s->curType = mmd_line_scan(s, options);

		if (s->cur > s->stop) {
			s->cur = s->stop;
		}

		if (s->c_end) {
			// We have an end point to the content before the actual eol/eof
			return mmd_line_node_new(s->curType, s->start - s->text, s->cur - s->start, s->c_start - s->start, s->c_end - s->c_start);
		} else {
			// Content ends with eol/eof
			return mmd_line_node_new(s->curType, s->start - s->text, s->cur - s->start, s->c_start - s->start, s->cur - s->c_start);
		}
	} else {
		return NULL;
	}
}


static mmd_line_node scanner_next_line_vector(vector_line_node * v, Scanner * s, uint32_t options) {
	mmd_line_node l = {0};

	if (v && s->cur < s->stop) {
		s->curType = mmd_line_scan(s, options);

		if (s->cur > s->stop) {
			s->cur = s->stop;
		}

		if (s->c_end) {
			// We have an end point to the content before the actual eol/eof
			mmd_line_node n = {
				{
					s->curType,
					0,
					s->start - s->text,
					s->cur - s->start,
					NULL,
					NULL,
					NULL,
					NULL,
				},
				s->c_start - s->start,
				s->c_end - s->c_start
			};

			l = n;
		} else {
			// Content ends with eol/eof
			mmd_line_node n = {
				{
					s->curType,
					0,
					s->start - s->text,
					s->cur - s->start,
					NULL,
					NULL,
					NULL,
					NULL,
				},
				s->c_start - s->start,
				s->cur - s->c_start,
			};

			l = n;
		}
	}

	return l;
}


static int accept_blockquote_line(mmd_node ** l) {
	if (*l == NULL) {
		return 0;
	}

	switch ((*l)->type) {
		case LINE_BLOCKQUOTE:
		case LINE_PLAIN:
		case LINE_SETEXT_1:
		case LINE_INDENTED_TAB:
		case LINE_INDENTED_SPACE:
			return 1;

		default:
			return 0;
	}
}


static int accept_para_line(mmd_node ** l) {
	if (*l == NULL) {
		return 0;
	}

	switch ((*l)->type) {
		case LINE_ATX_1:
		case LINE_ATX_2:
		case LINE_ATX_3:
		case LINE_ATX_4:
		case LINE_ATX_5:
		case LINE_ATX_6:
		case LINE_BLOCKQUOTE:
		case LINE_DEFINITION:
		case LINE_EMPTY:
		case LINE_FENCE_BACKTICK_3:
		case LINE_FENCE_BACKTICK_4:
		case LINE_FENCE_BACKTICK_5:
		case LINE_FENCE_BACKTICK_START_3:
		case LINE_FENCE_BACKTICK_START_4:
		case LINE_FENCE_BACKTICK_START_5:
		case LINE_HTML_BLOCK:
		case LINE_LIST_BULLETED:
		case LINE_LIST_ENUMERATED:
		case LINE_SETEXT_1:
		case LINE_SETEXT_2:
			return 0;

		default:
			return 1;
	}
}


static int accept_chunk_line(mmd_node ** l) {
	if (*l == NULL) {
		return 0;
	}

	switch ((*l)->type) {
		case LINE_ATX_1:
		case LINE_ATX_2:
		case LINE_ATX_3:
		case LINE_ATX_4:
		case LINE_ATX_5:
		case LINE_ATX_6:
		case LINE_BLOCKQUOTE:
		case LINE_DEF_ABBREVIATION:
		case LINE_DEF_CITATION:
		case LINE_DEF_FOOTNOTE:
		case LINE_DEF_GLOSSARY:
		case LINE_DEF_LINK:
		case LINE_DEFINITION:
		case LINE_EMPTY:
		case LINE_FENCE_BACKTICK_3:
		case LINE_FENCE_BACKTICK_4:
		case LINE_FENCE_BACKTICK_5:
		case LINE_FENCE_BACKTICK_START_3:
		case LINE_FENCE_BACKTICK_START_4:
		case LINE_FENCE_BACKTICK_START_5:
		case LINE_HTML_BLOCK:
		case LINE_LIST_BULLETED:
		case LINE_LIST_ENUMERATED:

		// case LINE_SETEXT_1:
		case LINE_SETEXT_2:
			return 0;

		default:
			return 1;
	}
}


static int accept_empty_line(mmd_node ** l) {
	if (*l == NULL) {
		return 0;
	}

	switch ((*l)->type) {
		case LINE_EMPTY:
			return 1;

		default:
			return 0;
	}
}


static int accept_fence_line(mmd_node ** l, int level) {
	if (*l == NULL) {
		return 0;
	}

	switch ((*l)->type) {
		case LINE_FENCE_BACKTICK_3:
		case LINE_FENCE_BACKTICK_4:
		case LINE_FENCE_BACKTICK_5:
			if ((*l)->type >= LINE_FENCE_BACKTICK_3 + level - 3) {
				return 0;
			}

		default:
			return 1;
	}
}


static int accept_html_line(mmd_node ** l) {
	if (*l == NULL) {
		return 0;
	}

	switch ((*l)->type) {
		case LINE_EMPTY:
			return 0;

		case LINE_ATX_1:
		case LINE_ATX_2:
		case LINE_ATX_3:
		case LINE_ATX_4:
		case LINE_ATX_5:
		case LINE_ATX_6:
		case LINE_BLOCKQUOTE:
		case LINE_DEF_ABBREVIATION:
		case LINE_DEF_CITATION:
		case LINE_DEF_FOOTNOTE:
		case LINE_DEF_GLOSSARY:
		case LINE_DEF_LINK:
		case LINE_DEFINITION:
		case LINE_FENCE_BACKTICK_3:
		case LINE_FENCE_BACKTICK_4:
		case LINE_FENCE_BACKTICK_5:
		case LINE_FENCE_BACKTICK_START_3:
		case LINE_FENCE_BACKTICK_START_4:
		case LINE_FENCE_BACKTICK_START_5:
		case LINE_START_COMMENT:
		case LINE_STOP_COMMENT:

		// case LINE_SETEXT_1:
		case LINE_SETEXT_2:
			//return 0;
			return 1;

		default:
			return 1;
	}
}


static int accept_html_comment_line(mmd_node ** l) {
	if (*l == NULL) {
		return 0;
	}

	switch ((*l)->type) {
		case LINE_STOP_COMMENT:
			return 0;

		case LINE_ATX_1:
		case LINE_ATX_2:
		case LINE_ATX_3:
		case LINE_ATX_4:
		case LINE_ATX_5:
		case LINE_ATX_6:
		case LINE_BLOCKQUOTE:
		case LINE_DEF_ABBREVIATION:
		case LINE_DEF_CITATION:
		case LINE_DEF_FOOTNOTE:
		case LINE_DEF_GLOSSARY:
		case LINE_DEF_LINK:
		case LINE_DEFINITION:
		case LINE_EMPTY:
		case LINE_FENCE_BACKTICK_3:
		case LINE_FENCE_BACKTICK_4:
		case LINE_FENCE_BACKTICK_5:
		case LINE_FENCE_BACKTICK_START_3:
		case LINE_FENCE_BACKTICK_START_4:
		case LINE_FENCE_BACKTICK_START_5:
		case LINE_HR:
		case LINE_HTML:
		case LINE_HTML_BLOCK:
		case LINE_START_COMMENT:

		// case LINE_SETEXT_1:
		case LINE_SETEXT_2:

		//return 0;

		default:
			return 1;
	}
}


static int accept_indented_line(mmd_node ** l) {
	if (*l == NULL) {
		return 0;
	}

	switch ((*l)->type) {
		case LINE_INDENTED_SPACE:
		case LINE_INDENTED_TAB:
			return 1;

		default:
			return 0;
	}
}


static int accept_table_line(mmd_node ** l) {
	if (*l == NULL) {
		return 0;
	}

	switch ((*l)->type) {
		case LINE_TABLE:
		case LINE_TABLE_SEPARATOR:
			return 1;

		default:
			return 0;
	}
}


static int accept_tail_line(mmd_node ** l) {
	if (*l == NULL) {
		return 0;
	}

	switch ((*l)->type) {
		case LINE_BLOCKQUOTE:
		case LINE_DEF_ABBREVIATION:
		case LINE_DEF_CITATION:
		case LINE_DEF_FOOTNOTE:
		case LINE_DEF_GLOSSARY:
		case LINE_DEF_LINK:
		case LINE_DEFINITION:
		case LINE_EMPTY:
		case LINE_LIST_BULLETED:
		case LINE_LIST_ENUMERATED:

		// case LINE_SETEXT_1:
		case LINE_SETEXT_2:
		case LINE_ATX_1:
		case LINE_ATX_2:
		case LINE_ATX_3:
		case LINE_ATX_4:
		case LINE_ATX_5:
		case LINE_ATX_6:
			return 0;

		default:
			return 1;
	}
}


static mmd_node * mmd_node_feed_new_parent(mmd_node ** l, unsigned char type, mmd_node_pool * p) {
	mmd_node * next = (*l)->next;
	(*l)->next = NULL;

	mmd_node * b = mmd_node_new_parent(p, *l, type);
	*l = next;
	return b;
}


static mmd_node * mmd_node_feed_chain(mmd_node * b, mmd_node * child) {
	mmd_node * next = child->next;
	child->next = NULL;
	mmd_node_append_child(b, child);
	return next;
}


static mmd_node * block_empty(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_EMPTY, p);

	while (accept_empty_line(l)) {
		*l = mmd_node_feed_chain(b, *l);
	}

	return b;
}


static mmd_node * block_general(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_GENERAL, p);

//	while ((*l) && (*l)->type != LINE_EMPTY) {
	while (accept_tail_line(l)) {
		*l = mmd_node_feed_chain(b, *l);
	}

	return b;
}


static int block_append_tail(mmd_node ** l, mmd_node * b, mmd_node_pool * p) {
	int has_blank_lines = 0;

	while ((*l)) {
		switch ((*l)->type) {
			case LINE_EMPTY: {
				mmd_node * cache = block_empty(l, p);

				if (accept_indented_line(l)) {
					mmd_node_graft(cache, b);

					if (p == NULL) {
						// Not needed with mmd_node_pool
						free(cache);
					}

					has_blank_lines = 1;
				} else {
					b->next = cache;
					b->tail = cache;
					return has_blank_lines;
				}
			}

			break;

			case LINE_INDENTED_SPACE:
			case LINE_INDENTED_TAB: {
				mmd_node * cache = block_general(l, p);
				mmd_node_graft(cache, b);

				if (p == NULL) {
					// Not needed with mmd_node_pool
					free(cache);
				}
			}
			break;

			default:
				return has_blank_lines;

		}
	}

	return has_blank_lines;
}


static mmd_node * block_atx(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_feed_new_parent(l, (*l)->type - LINE_ATX_1 + BLOCK_H1, p);

	return b;
}


static mmd_node * block_blockquote(mmd_node ** l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_BLOCKQUOTE, p);

	while (accept_blockquote_line(l)) {
		*l = mmd_node_feed_chain(b, *l);
	}

	// Recursively parse blockquote
	c->is_blockquote++;
	b->child = recursive_blockquote_parse(b->child, p, &text[b->start], len - b->start, c, options);
	c->is_blockquote--;

	return b;
}


static mmd_node * block_code_fenced(mmd_node ** l, mmd_node_pool * p) {
	int level = 0;

	switch ((*l)->type) {
		case LINE_FENCE_BACKTICK_3:
		case LINE_FENCE_BACKTICK_4:
		case LINE_FENCE_BACKTICK_5:
			level = 3 + (*l)->type - LINE_FENCE_BACKTICK_3;
			break;

		case LINE_FENCE_BACKTICK_START_3:
		case LINE_FENCE_BACKTICK_START_4:
		case LINE_FENCE_BACKTICK_START_5:
			level = 3 + (*l)->type - LINE_FENCE_BACKTICK_START_3;
			break;
	}

	(*l)->type = CODE_FENCE_LINE;

	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_CODE_FENCED, p);

	while (accept_fence_line(l, level)) {
		*l = mmd_node_feed_chain(b, *l);
	}

	if (*l) {
		switch ((*l)->type) {
			case LINE_FENCE_BACKTICK_3:
			case LINE_FENCE_BACKTICK_4:
			case LINE_FENCE_BACKTICK_5:
				(*l)->type = CODE_FENCE_LINE;
				*l = mmd_node_feed_chain(b, *l);
				break;
		}
	}

	return b;
}


static mmd_node * block_code_indented(mmd_node ** l, mmd_node_pool * p, const char * text) {
	mmd_node * b = mmd_node_new_parent(p, NULL, BLOCK_CODE_INDENTED);
	b->start = (*l)->start;

	while (accept_indented_line(l)) {
		// Adjust c_start and c_len for indentation
		if (strncmp("\t", &text[(*l)->start + ((mmd_line_node *)(*l))->c_start], 1) == 0) {
			((mmd_line_node *)(*l))->c_start++;
			((mmd_line_node *)(*l))->c_len--;
		} else if (strncmp("    ", &text[(*l)->start + ((mmd_line_node *)(*l))->c_start], 4) == 0) {
			((mmd_line_node *)(*l))->c_start += 4;
			((mmd_line_node *)(*l))->c_len -= 4;
		}

		*l = mmd_node_feed_chain(b, *l);

		if ((*l) != NULL) {
			switch ((*l)->type) {
				case LINE_EMPTY: {
					mmd_node * cache = block_empty(l, p);

					if (accept_indented_line(l)) {
						mmd_node_graft(cache, b);

						if (p == NULL) {
							// Not needed with mmd_node_pool
							free(cache);
						}
					} else {
						b->next = cache;
						return b;
					}
				}
				break;
			}
		}
	}

	return b;
}


static mmd_node * block_definition(mmd_node ** l, mmd_node_pool * p, const char * text, read_ctx * c, uint32_t options) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_DEFINITION, p);

	while (accept_tail_line(l)) {
		*l = mmd_node_feed_chain(b, *l);
	}

	block_append_tail(l, b, p);

	// Recursively parse definition
	c->is_definition++;
	b->child = recursive_indent_parse((mmd_line_node *) b->child, p, &text[b->start], b->len, c, options);
	c->is_definition--;

	return b;
}


static mmd_node * block_hr(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_HR, p);

	return b;
}


static mmd_node * block_html_comment(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_HTML, p);

	while (accept_html_comment_line(l)) {
		*l = mmd_node_feed_chain(b, *l);
	}

	if ((*l) && (*l)->type == LINE_STOP_COMMENT) {
		*l = mmd_node_feed_chain(b, *l);
	}

	return b;
}


static mmd_node * block_html(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_HTML, p);

	while (accept_html_line(l)) {
		*l = mmd_node_feed_chain(b, *l);
	}

	return b;
}


static mmd_node * block_list_item(mmd_node ** l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_LIST_ITEM_TIGHT, p);

	while (accept_chunk_line(l)) {
		*l = mmd_node_feed_chain(b, *l);
	}

	if (b->next && b->next->type == BLOCK_EMPTY) {
		b->type = BLOCK_LIST_ITEM;
	}

	if (block_append_tail(l, b, p)) {
		b->type = BLOCK_LIST_ITEM;
	}

	// Recursively parse list item
	c->is_list_item++;
	b->child = recursive_indent_parse((mmd_line_node *)b->child, p, &text[b->start], len - b->start, c, options);
	c->is_list_item--;

	return b;
}


static mmd_node * block_list_bulleted(mmd_node ** l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * b = mmd_node_new_parent(p, NULL, BLOCK_LIST_BULLETED);
	b->start = (*l)->start;

	while ((*l) && (*l)->type == LINE_LIST_BULLETED) {
		mmd_node * cache = block_list_item(l, p, text, len, c, options);

		mmd_node_append_child(b, cache);
	}

	return b;
}


static mmd_node * block_list_enumerated(mmd_node ** l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * b = mmd_node_new_parent(p, NULL, BLOCK_LIST_ENUMERATED);
	b->start = (*l)->start;

	while ((*l) && (*l)->type == LINE_LIST_ENUMERATED) {
		mmd_node * cache = block_list_item(l, p, text, len, c, options);

		mmd_node_append_child(b, cache);
	}

	return b;
}


static mmd_node * block_meta(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_META, p);

	while ((*l) && (*l)->type != LINE_EMPTY) {
		*l = mmd_node_feed_chain(b, *l);
	}

	return b;
}


static mmd_node * block_meta_yaml(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_META, p);

	while ((*l) && (((*l)->type != LINE_EMPTY) && ((*l)->type != LINE_SETEXT_2))) {
		*l = mmd_node_feed_chain(b, *l);
	}

	return b;
}


/// Promote lines within a block to their own child block of specified type
static void block_promote_lines(mmd_node * b, unsigned char type, mmd_node_pool * p) {
	mmd_node * l = b->child;
	b->child = NULL;
	mmd_node * new;

	while (l) {
		new = mmd_node_feed_new_parent(&l, type, p);
		new->start += b->start;
		mmd_node_append_child(b, new);
	}
}


static mmd_node * block_para(mmd_node ** l, mmd_node_pool * p, const char * text, read_ctx * c, uint32_t options) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_PARA, p);

	while (accept_para_line(l)) {
		*l = mmd_node_feed_chain(b, *l);
	}

	if ((*l) != NULL) {
		switch ((*l)->type) {
			case LINE_DEFINITION: {
				// Definition converts para into a definition
				b->type = BLOCK_DEFLIST;

				// Promote each term line into a block
				block_promote_lines(b, BLOCK_TERM, p);

				// Addend definitions
				while ((*l) && (*l)->type == LINE_DEFINITION) {
					mmd_node_append_child(b, block_definition(l, p, text, c, options));
				}
			}

			return b;

			case LINE_SETEXT_1:
			case LINE_SETEXT_2:
				// Setext marker converts para into Setext header
				b->type = (*l)->type - LINE_SETEXT_1 + BLOCK_SETEXT_1;
				*l = mmd_node_feed_chain(b, *l);
				return b;
		}
	}

	return b;
}


static mmd_node * block_reference_def_a(mmd_node ** l, mmd_node_pool * p, uint32_t options) {
	mmd_node * b = NULL;

	switch ((*l)->type) {
		case LINE_DEF_ABBREVIATION:
			if (options & MMD_OPTION_COMPATIBILITY) {
				b = mmd_node_feed_new_parent(l, BLOCK_DEF_LINK, p);
			} else {
				b = mmd_node_feed_new_parent(l, BLOCK_DEF_ABBREVIATION, p);
			}

			break;

		case LINE_DEF_LINK:
			b = mmd_node_feed_new_parent(l, BLOCK_DEF_LINK, p);
			break;
	}

	if (b) {
		while (accept_chunk_line(l)) {
			*l = mmd_node_feed_chain(b, *l);
		}
	}

	return b;
}


static mmd_node * block_endnote_definition(mmd_node ** l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * b = NULL;

	switch ((*l)->type) {
		case LINE_DEF_CITATION:
			b = mmd_node_feed_new_parent(l, BLOCK_DEF_CITATION, p);
			break;

		case LINE_DEF_FOOTNOTE:
			b = mmd_node_feed_new_parent(l, BLOCK_DEF_FOOTNOTE, p);
			break;

		case LINE_DEF_GLOSSARY:
			b = mmd_node_feed_new_parent(l, BLOCK_DEF_GLOSSARY, p);
			break;
	}

	while (accept_chunk_line(l)) {
		*l = mmd_node_feed_chain(b, *l);
	}

	// These definitions may have an indented tail
	block_append_tail(l, b, p);

	// Recursively parse endnote content
	endnote_def * e = NULL;

	c->is_endnote++;
	b->child = recursive_endnote_parse(&e, b->child, p, &text[b->start], len - b->start, c, options);
	c->is_endnote--;

	if (e) {
		e->text = &text[b->start];
		e->len = b->len;

		e->content_node = b->child;

		switch (b->type) {
			case BLOCK_DEF_CITATION:
				read_ctx_store_cite(c, e);
				break;

			case BLOCK_DEF_FOOTNOTE:
				read_ctx_store_note(c, e);
				break;

			case BLOCK_DEF_GLOSSARY: {
				mmd_node * t = e->content_node->content;

				while (t && t->type != TOKEN_PAIR_BRACKET_GLOSSARY) {
					t = t->next;
				}

				if (t && t->type == TOKEN_PAIR_BRACKET_GLOSSARY) {
					e->title_node = t->child;
					// Clean up incorrectly changed token types
					mmd_node * walker = e->title_node;

					while (walker) {
						switch (walker->type) {
							case TOKEN_LIST_MARKER:
							case TOKEN_BLOCKQUOTE_MARKER:
							case TOKEN_DEFLIST_COLON:
							case TOKEN_ATX_MARKER:
								walker->type = TOKEN_TEXT;
								break;

							default:
								break;
						}

						walker = walker->next;
					}
				}
			}

			read_ctx_store_glos(c, e);
			break;
		}
	} else {
		// Should something different be done here?
		b->type = BLOCK_PARA;
	}

	return b;
}


static mmd_node * block_table_section(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_TABLE_SECTION, p);

	while ((*l) && (((*l)->type == LINE_TABLE) || ((*l)->type == LINE_TABLE_SEPARATOR))) {
		*l = mmd_node_feed_chain(b, *l);
	}

	return b;
}


static mmd_node * block_table(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_new_parent(p, NULL, BLOCK_TABLE_HEADER);
	b->start = (*l)->start;


	// Match header rows (optional)
	while ((*l) && (*l)->type == LINE_TABLE) {
		*l = mmd_node_feed_chain(b, *l);
	}

	// Match table separator row (required)
	if ((*l) && (*l)->type == LINE_TABLE_SEPARATOR) {
		*l = mmd_node_feed_chain(b, *l);
	} else {
		b->type = BLOCK_PARA;

		while (accept_para_line(l)) {
			*l = mmd_node_feed_chain(b, *l);
		}

		return b;
	}

	// Match table body consisting of one or more table sections (required)
	if ((*l) && (*l)->type == LINE_TABLE) {
		mmd_node * last = b;

		while (accept_table_line(l)) {
			last->next = block_table_section(l, p);
			last = last->next;

			if ((*l) && (*l)->type == LINE_EMPTY) {
				mmd_node * cache = mmd_node_feed_new_parent(l, BLOCK_EMPTY, p);

				if (!accept_table_line(l)) {
					b = mmd_node_new_parent(p, b, BLOCK_TABLE);
					b->next = cache;

					return b;
				} else {
					mmd_node_graft(cache, b);

					if (p == NULL) {
						// Not needed with mmd_node_pool
						free(cache);
					}
				}
			}
		}
	} else {
		b->type = BLOCK_PARA;

		return b;
	}

	return mmd_node_new_parent(p, b, BLOCK_TABLE);
}


static mmd_node * block_toc(mmd_node ** l, mmd_node_pool * p) {
	mmd_node * b = mmd_node_feed_new_parent(l, BLOCK_TOC, p);

	return b;
}


static mmd_node * block(mmd_node ** l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * b = NULL;

	if (*l == NULL) {
		return NULL;
	}

	switch ((*l)->type) {
		case LINE_ATX_1:
		case LINE_ATX_2:
		case LINE_ATX_3:
		case LINE_ATX_4:
		case LINE_ATX_5:
		case LINE_ATX_6: {
			mmd_line_node * line = (mmd_line_node *) *l;

			b = block_atx(l, p);

			mmd_parse_tokens_block(b, &text[b->start], c, p, options);

			if (!(options & MMD_OPTION_COMPATIBILITY)) {
				size_t c_len = line->c_len;

				while (c_len && char_is_whitespace_or_line_ending(text[b->start + line->c_start + c_len - 1])) {
					c_len--;
				}

				// These next two passes are because lines with '#' inside the line catch the first trailing '#' as part of the content
				while (c_len && text[b->start + line->c_start + c_len - 1] == '#') {
					c_len--;
				}

				while (c_len && char_is_whitespace_or_line_ending(text[b->start + line->c_start + c_len - 1])) {
					c_len--;
				}

				mmd_node * label = mask_manual_label_token(b);

				if (label) {
					// Use manually specified label
					char * key = html_id_from_text(&text[b->start + label->start + 1], label->next->start - label->start - 1, true);

					read_ctx_store_internal_link_key(c, key, strlen(key));
					read_ctx_store_header(c, &text[b->start], b->len, b, key, strlen(key), line->c_start, c_len);

					free(key);
				} else if (!(options & MMD_OPTION_RANDOM_HEADER_ID)) {
					// Use automatic label
					if (b->content->next) {
						char * key = html_id_from_text(&text[b->start + b->content->next->start], b->child->len - b->content->next->start, true);

						read_ctx_store_internal_link_key(c, key, strlen(key));
						read_ctx_store_header(c, &text[b->start], b->len, b, key, strlen(key), line->c_start, c_len);

						free(key);
					}
				} else {
					// Use random id
					char key[6] = {0};
					snprintf(key, 6, "%d", xorshift16(c->random_header_seed + c->header_stack->size));

					read_ctx_store_internal_link_key(c, key, strlen(key));
					read_ctx_store_header(c, &text[b->start], b->len, b, key, strlen(key), line->c_start, c_len);
				}
			}
		}

		break;

		case LINE_BLOCKQUOTE:
			b = block_blockquote(l, p, text, len, c, options);
			break;

		case LINE_DEF_ABBREVIATION:
			b = block_reference_def_a(l, p, options);
			read_ctx_store_abbr(c, scan_ref_abbr(&text[b->start], b->len));
			break;

		case LINE_DEF_LINK:
			b = block_reference_def_a(l, p, options);
			read_ctx_store_link(c, scan_ref_link(&text[b->start], b->len));
			break;

		case LINE_DEF_CITATION:
		case LINE_DEF_FOOTNOTE:
		case LINE_DEF_GLOSSARY:
			if (c->is_endnote) {
				b = block_para(l, p, text, c, options);
			} else {
				b = block_endnote_definition(l, p, text, len, c, options);
			}

			break;

		case LINE_EMPTY:
			b = block_empty(l, p);
			break;

		case LINE_FENCE_BACKTICK_3:
		case LINE_FENCE_BACKTICK_4:
		case LINE_FENCE_BACKTICK_5:
		case LINE_FENCE_BACKTICK_START_3:
		case LINE_FENCE_BACKTICK_START_4:
		case LINE_FENCE_BACKTICK_START_5:
			b = block_code_fenced(l, p);
			break;

		case LINE_YAML:
			if (c->allow_meta) {
				b = block_meta_yaml(l, p);
				mmd_parse_meta_block(&text[b->start + b->child->len], b->len - b->child->len, c);

				// Add trailing YAML line (if present) *after* we parse the metadata block
				if ((*l) && ((*l)->type == LINE_SETEXT_2)) {
					(*l)->type = LINE_YAML;
					*l = mmd_node_feed_chain(b, *l);
				}

				return b;
			}

		case LINE_HR:

		// case LINE_SETEXT_1:
		case LINE_SETEXT_2:
			b = block_hr(l, p);
			mmd_parse_tokens_block(b, &text[b->start], c, p, options);
			break;

		case LINE_HTML_BLOCK:
		case LINE_HTML_BLOCKISH:
			b = block_html(l, p);
			break;

		case LINE_INDENTED_SPACE:
		case LINE_INDENTED_TAB:
			b = block_code_indented(l, p, text);
			break;

		case LINE_START_COMMENT:
			b = block_html_comment(l, p);
			break;

		case LINE_LIST_BULLETED:
			b = block_list_bulleted(l, p, text, len, c, options);
			flag_list_loose(b);
			break;

		case LINE_LIST_ENUMERATED:
			b = block_list_enumerated(l, p, text, len, c, options);
			flag_list_loose(b);
			break;

		case LINE_TABLE:
		case LINE_TABLE_SEPARATOR:
			if (!(options & MMD_OPTION_COMPATIBILITY)) {
				b = block_table(l, p);

				if (b->type == BLOCK_TABLE) {
					mmd_parse_tokens_table(b, &text[b->start], c, p, options);
				} else {
					mmd_parse_tokens_block(b, &text[b->start], c, p, options);
				}
			} else {
				b = block_para(l, p, text, c, options);
			}

			break;

		case LINE_TOC:
			b = block_toc(l, p);
			break;

		case LINE_META:
			if (c->allow_meta) {
				b = block_meta(l, p);
				mmd_parse_meta_block(&text[b->start], b->len, c);
				return b;
			}

		default:
			b = block_para(l, p, text, c, options);

			switch (b->type) {
				case BLOCK_DEFLIST:
					mmd_parse_tokens_deflist(b, &text[b->start], c, p, options);
					break;

				default:
					mmd_parse_tokens_block(b, &text[b->start], c, p, options);
					break;
			}

			break;
	}

	return b;
}


static void block_check(mmd_node * b, mmd_node * last, const char * text, read_ctx * c, uint32_t options) {
	if (b) {
		switch (b->type) {
			case BLOCK_PARA: {
				if (last && last->type == BLOCK_TABLE) {
					char * label = table_label(last, text);

					if (label) {
						read_ctx_store_internal_link(c, label, strlen(label), false);
					}

					free(label);
				}

				if (!(options & MMD_OPTION_COMPATIBILITY) && (b->content && b->content->type == TOKEN_PAIR_BRACKET_IMAGE)) {
					// Check for figure
					mmd_node * walker = b->content->next;

					if (walker && walker->type == TOKEN_BRACKET_RIGHT) {
						walker = walker->next;
					}

					if (walker && walker->type == TOKEN_PAIR_BRACKET) {
						walker = walker->next;

						if (walker && walker->type == TOKEN_BRACKET_RIGHT) {
							walker = walker->next;
						}
					} else if (walker && walker->type == TOKEN_PAIR_PAREN) {
						walker = walker->next;

						if (walker && walker->type == TOKEN_PAREN_RIGHT) {
							walker = walker->next;
						}
					} else if (walker && walker->type == TOKEN_PAIR_BRACKET_EMPTY) {
						walker = walker->next;
					}

					if (walker && ((walker->type == TOKEN_NL) || (walker->type == TOKEN_LINEBREAK))) {
						walker = walker->next;
					}

					if (walker == NULL) {
						b->type = BLOCK_FIGURE;
					}
				}
			}
			break;

			case BLOCK_SETEXT_1:
			case BLOCK_SETEXT_2:
				if (!(options & MMD_OPTION_COMPATIBILITY)) {
					size_t c_len = b->child->tail->start;

					while (c_len && char_is_whitespace_or_line_ending(text[b->start + c_len - 1])) {
						c_len--;
					}

					mmd_node * label = mask_manual_label_token(b);

					if (label) {
						// Use manually specified label
						char * key = html_id_from_text(&text[b->start + label->start + 1], label->next->start - label->start - 1, true);

						read_ctx_store_internal_link_key(c, key, strlen(key));
						read_ctx_store_header(c, &text[b->start], b->len, b, key, strlen(key), 0, c_len);

						free(key);
					} else if (!(options & MMD_OPTION_RANDOM_HEADER_ID)) {
						// Use automatic label
						char * key = html_id_from_text(&text[b->start], b->len - b->child->tail->len, true);

						read_ctx_store_internal_link_key(c, key, strlen(key));
						read_ctx_store_header(c, &text[b->start], b->len, b, key, strlen(key), 0, c_len);

						free(key);
					} else {
						// Use random id
						char key[6] = {0};
						snprintf(key, 6, "%d", xorshift16(c->random_header_seed + c->header_stack->size));

						read_ctx_store_internal_link_key(c, key, strlen(key));
						read_ctx_store_header(c, &text[b->start], b->len, b, key, strlen(key), 0, c_len);
					}
				}

				break;
		}
	}
}


static mmd_node * blocks(mmd_node ** l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * b, * n, * last;

	b = block(l, p, text, len, c, options);

	// Check for special cases
	block_check(b, NULL, text, c, options);

	last = b;

	while (last && last->next) {
		// Check for special cases
		block_check(last->next, last, text, c, options);

		last = last->next;
	}

	c->allow_meta = 0;

	while ((n = block(l, p, text, len, c, options))) {
		last->next = n;

		// Check for special cases
		block_check(n, last, text, c, options);

		last = n;

		while (last->next) {
			last = last->next;
		}
	}

	return b;
}


static mmd_node * block_only(mmd_node ** l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * b = NULL;

	if (*l == NULL) {
		return NULL;
	}

	switch ((*l)->type) {
		case LINE_ATX_1:
		case LINE_ATX_2:
		case LINE_ATX_3:
		case LINE_ATX_4:
		case LINE_ATX_5:
		case LINE_ATX_6:
			b = block_atx(l, p);
			break;

		case LINE_BLOCKQUOTE:
			b = block_blockquote(l, p, text, len, c, options);
			break;

		case LINE_DEF_ABBREVIATION:
		case LINE_DEF_LINK:
			b = block_reference_def_a(l, p, options);
			break;

		case LINE_DEF_CITATION:
		case LINE_DEF_FOOTNOTE:
		case LINE_DEF_GLOSSARY:
			if (options & MMD_OPTION_COMPATIBILITY) {
				b = block_reference_def_a(l, p, options);
			} else {
				b = block_endnote_definition(l, p, text, len, c, options);
			}

			break;

		case LINE_EMPTY:
			b = block_empty(l, p);
			break;

		case LINE_FENCE_BACKTICK_3:
		case LINE_FENCE_BACKTICK_4:
		case LINE_FENCE_BACKTICK_5:
		case LINE_FENCE_BACKTICK_START_3:
		case LINE_FENCE_BACKTICK_START_4:
		case LINE_FENCE_BACKTICK_START_5:
			b = block_code_fenced(l, p);
			break;

		case LINE_YAML:
			if (c->allow_meta) {
				b = block_meta(l, p);
				return b;
			}

		case LINE_HR:

		// case LINE_SETEXT_1:
		case LINE_SETEXT_2:
			b = block_hr(l, p);
			break;

		case LINE_HTML_BLOCK:
		case LINE_HTML_BLOCKISH:
			b = block_html(l, p);
			break;

		case LINE_INDENTED_SPACE:
		case LINE_INDENTED_TAB:
			b = block_code_indented(l, p, text);
			break;

		case LINE_START_COMMENT:
			b = block_html_comment(l, p);
			break;

		case LINE_LIST_BULLETED:
			b = block_list_bulleted(l, p, text, len, c, options);
			flag_list_loose(b);
			break;

		case LINE_LIST_ENUMERATED:
			b = block_list_enumerated(l, p, text, len, c, options);
			flag_list_loose(b);
			break;

		case LINE_TABLE:
		case LINE_TABLE_SEPARATOR:
			if (!(options & MMD_OPTION_COMPATIBILITY)) {
				b = block_table(l, p);
			} else {
				b = block_para(l, p, text, c, options);
			}

			break;

		case LINE_TOC:
			b = block_toc(l, p);
			break;

		case LINE_META:
			if (c->allow_meta) {
				b = block_meta(l, p);
				return b;
			}

		default:
			b = block_para(l, p, text, c, options);
			break;
	}

	return b;
}


static mmd_node * blocks_only(mmd_node ** l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * b, * n, * last;

	b = block_only(l, p, text, len, c, options);

	// Check for special cases
	block_check(b, NULL, text, c, options);

	last = b;

	while (last && last->next) {
		// Check for special cases
		block_check(last->next, last, text, c, options);

		last = last->next;
	}

	c->allow_meta = 0;

	while ((n = block_only(l, p, text, len, c, options))) {
		last->next = n;

		// Check for special cases
		block_check(n, last, text, c, options);

		last = n;

		while (last->next) {
			last = last->next;
		}
	}

	return b;
}


static void search_text(mmd_node * n, const char * text, ac * a, mmd_node_pool * p) {
	match * m = ac_search(a, AC_LEFTMOST | AC_LONGEST, (const unsigned char *) text, n->start, n->len);
	match * walker = m;

	while (walker) {
		// Ensure we matched a full word
		if ((walker->start + walker->len == n->start + n->len) ||
				(char_is_whitespace_or_line_ending_or_punctuation(text[walker->start + walker->len]))
		   ) {

			// Split match as new node
			mmd_node_split(p, n, walker->start - n->start);
			mmd_node_split(p, n->next, walker->len);

			n = n->next;

			switch (walker->type) {
				case TOKEN_PAIR_BRACKET_ABBREVIATION:
					n->type = TOKEN_TEXT_ABBREVIATION;
					break;

				case TOKEN_PAIR_BRACKET_GLOSSARY:
					n->type = TOKEN_TEXT_GLOSSARY;
					break;
			}

			n = n->next;
		}

		walker = walker->next;
	}

	match_free(m);
}


/// Recursive descent search
static void recursive_search(mmd_node * n, const char * text, ac * a, read_ctx * c, mmd_node_pool * p) {
	while (n) {
		switch (n->type) {
			case TOKEN_TEXT:
				// Search these node types
				search_text(n, text, a, p);
				break;

			case BLOCK_BLOCKQUOTE:
			case BLOCK_DEFINITION:
			case BLOCK_LIST_BULLETED:
			case BLOCK_LIST_BULLETED_LOOSE:
			case BLOCK_LIST_ENUMERATED:
			case BLOCK_LIST_ENUMERATED_LOOSE:
			case BLOCK_LIST_ITEM_TIGHT:
			case BLOCK_LIST_ITEM:
			case BLOCK_TABLE:
			case BLOCK_TABLE_HEADER:
			case BLOCK_TABLE_SECTION:
			case TOKEN_TABLE_CELL:
			case BLOCK_TERM:
			case TOKEN_PAIR_BRACKET:
			case TOKEN_PAIR_BRACKET_FOOTNOTE:

			case TOKEN_PAIR_BRACKET_LINK:
			case TOKEN_PAIR_BRACKET_IMAGE:
			case TOKEN_PAIR_STRONG:
			case TOKEN_PAIR_EMPH:

				// Descend into the children of these block types
				if (MMD_NODE_IS_BLOCK(n)) {
					recursive_search(n->child, &text[n->start], a, c, p);
				} else {
					recursive_search(n->child, text, a, c, p);
				}

				break;

			case BLOCK_PARA:
			case BLOCK_TABLE_ROW:
			case BLOCK_FIGURE:
				// Descend into the content of these block types
				recursive_search(n->content, &text[n->start], a, c, p);
				break;

			case BLOCK_EMPTY:
			case BLOCK_DEF_ABBREVIATION:
			case TOKEN_TEXT_ABBREVIATION:
			case TOKEN_PAIR_BRACKET_GLOSSARY:
				// Ignore these
				break;

			default:
				//mmd_node_describe(n, stderr, text, 0);
				break;
		}

		n = n->next;
	}
}


/// Locate abbreviations and glossary terms and flag them
static void global_search(mmd_node * d, const char * text, read_ctx * c, mmd_node_pool * p) {
	// Only search if we have something to look for
	if (!c->abbr_def_hash && !c->glos_def_hash) {
		return;
	}

	ac * act = ac_new(0);

	// Insert search terms
	abbr_def * a, * a_tmp;

	HASH_ITER(hh, c->abbr_def_hash, a, a_tmp) {
		ac_insert(act, a->key + 1, TOKEN_PAIR_BRACKET_ABBREVIATION);
		// Ensure abbreviation is marked as unused
		a->used = false;
	}

	endnote_def * e, * e_tmp;

	HASH_ITER(hh, c->glos_def_hash, e, e_tmp) {
		ac_insert(act, e->key + 1, TOKEN_PAIR_BRACKET_GLOSSARY);
		e->index = 0;
		e->used = false;
	}

	ac_prepare(act, AC_LEFTMOST | AC_LONGEST);
	// ac_to_graphviz(act, stderr);

	// mmd_node_tree_describe(d, stderr, text, 0);
	recursive_search(d, text, act, c, p);
	// mmd_node_tree_describe(d, stderr, text, 0);

	ac_free(act);
}


static mmd_node * doc(mmd_node * l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * doc;

	// YAML Metadata?
	if (l && l->type == LINE_SETEXT_2) {
		l->type = LINE_YAML;
	}

	if (options & MMD_OPTION_BLOCKS_ONLY) {
		doc = blocks_only(&l, p, text, len, c, options);
	} else {
		doc = blocks(&l, p, text, len, c, options);
	}

	// Now that we have parsed the document, we need to process for abbreviations and glossary terms
	if (!(options & MMD_OPTION_COMPATIBILITY)) {
		global_search(doc, text, c, p);
	}

	return doc;
}


static mmd_node * recursive_indent_parse(mmd_line_node * l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_line_node * line = l;

	// Skip list marker from first line and reassign type
	Scanner s = mmd_scanner(&text[line->c_start], line->c_len);
	line->general.type = mmd_line_scan(&s, options);

	// Adjust content range
	if (line->general.type) {
		line->c_start = (s.c_start - text) - line->general.start;
		line->c_len = s.cur - s.c_start;
	}

	line->general.tail = (mmd_node *) line;
	line = (mmd_line_node *) line->general.next;

	while (line) {
		line->general.tail = (mmd_node *) line;

		// Deindent by adjusting c_start and c_len; reassign line type
		if (strncmp("\t", &text[line->general.start + line->c_start], 1) == 0) {
			line->c_start++;
			line->c_len--;
		} else if (strncmp("    ", &text[line->general.start + line->c_start], 4) == 0) {
			line->c_start += 4;
			line->c_len -= 4;
		} else {
			// If no adjustment, no change to line type
			// Except Setext
			switch (line->general.type) {
				case LINE_SETEXT_1:
				case LINE_SETEXT_2:
					line->general.type = LINE_PLAIN;
					break;
			}

			line = (mmd_line_node *) line->general.next;
			continue;
		}

		s = mmd_scanner(&text[line->general.start + line->c_start], line->c_len);
		line->general.type = mmd_line_scan(&s, options);
		line->c_start = (s.c_start - text) - line->general.start;

		if (s.c_end) {
			line->c_len = s.c_end - s.c_start;
		} else {
			line->c_len = s.cur - s.c_start;
		}

		line = (mmd_line_node *) line->general.next;
	}

	if (options & MMD_OPTION_BLOCKS_ONLY) {
		return blocks_only((mmd_node **)&l, p, text, len, c, options);
	} else {
		return blocks((mmd_node **)&l, p, text, len, c, options);
	}
}


static mmd_node * recursive_blockquote_parse(mmd_node * l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_node * w = l;
	mmd_line_node * line = (mmd_line_node *) l;

	Scanner s;

	while (w) {
		// Skip blockquote marker if present and reassign line type
		switch (w->type) {
			case LINE_BLOCKQUOTE: {
				const char * content = &text[w->start + line->c_start];

				if (!strncmp(content, ">", 1)) {
					line->c_start += 1;
					line->c_len -= 1;
				} else if (!strncmp(content, " >", 2)) {
					line->c_start += 2;
					line->c_len -= 2;
				} else if (!strncmp(content, "  >", 3)) {
					line->c_start += 3;
					line->c_len -= 3;
				} else if (!strncmp(content, "   >", 4)) {
					line->c_start += 4;
					line->c_len -= 4 ;
				} else {
					s = mmd_scanner(&text[w->start + line->c_start], line->c_len);
					w->type = mmd_line_scan(&s, options);

					if (w->type) {
						((mmd_line_node *)w)->c_start = (s.c_start - text) - w->start;
						((mmd_line_node *)w)->c_len = s.cur - s.c_start;
					}
				}
			}
			break;

			// Reassign certain line types to "disable" them
			case LINE_SETEXT_1:
				w->type = LINE_PLAIN;
				break;

			default:
				break;
		}

		w->tail = w;
		w = w->next;
		line = (mmd_line_node *)w;
	}

	if (options & MMD_OPTION_BLOCKS_ONLY) {
		return blocks_only(&l, p, text, len, c, options);
	} else {
		return blocks(&l, p, text, len, c, options);
	}
}


static mmd_node * recursive_endnote_parse(endnote_def ** e, mmd_node * l, mmd_node_pool * p, const char * text, size_t len, read_ctx * c, uint32_t options) {
	mmd_line_node * line = (mmd_line_node *) l;

	// Create endnote_def
	*e = scan_ref_endnote(&text[l->start], l->len);

	if (*e == NULL) {
		return NULL;
	}

	// Skip endnote label from first line and reassign type
	Scanner s = mmd_scanner(&text[line->c_start], line->c_len);
	line->general.type = mmd_line_scan(&s, options);

	line->general.tail = (mmd_node *) line;
	line = (mmd_line_node *) line->general.next;

	while (line) {
		line->general.tail = (mmd_node *) line;

		// Deindent and reassign line type
		if (strncmp("\t", &text[line->general.start + line->c_start], 1) == 0) {
			line->c_start++;
			line->c_len--;
		} else if (strncmp("    ", &text[line->general.start + line->c_start], 4) == 0) {
			line->c_start += 4;
			line->c_len -= 4;
		} else {
			// If no adjustment, no change to line type
			line = (mmd_line_node *) line->general.next;
			continue;
		}

		s = mmd_scanner(&text[line->general.start + line->c_start], line->c_len);
		line->general.type = mmd_line_scan(&s, options);
		line->c_start = (s.c_start - text) - line->general.start;
		line->c_len = s.cur - s.c_start;

		line = (mmd_line_node *) line->general.next;
	}

	if (options & MMD_OPTION_BLOCKS_ONLY) {
		return blocks_only(&l, p, text, len, c, options);
	} else {
		return blocks(&l, p, text, len, c, options);
	}
}


static int skip_bom(const char * text) {
	int skip = 0;

	// Strip UTF-8 BOM
	if (strncmp(text, "\xef\xbb\xbf", 3) == 0) {
		skip += 3;
	}

	// Strip UTF-16 BOMs
	if (strncmp(&text[skip], "\xef\xff", 2) == 0) {
		skip += 2;
	}

	if (strncmp(&text[skip], "\xfe\xff", 2) == 0) {
		skip += 2;
	}

	return skip;
}


/// Given source text, return a tree of the line structure
mmd_line_node * mmd_scan_lines(const char * text, size_t len, uint32_t options) {
	// re2c scanner
	Scanner s = mmd_scanner(text, len);
	s.cur += skip_bom(text);

	mmd_line_node * l, * last;

	l = scanner_next_line(&s, options);
	last = l;

	while (last) {
		last->general.next = (mmd_node *) scanner_next_line(&s, options);
		last = (mmd_line_node *) last->general.next;
	}

	return l;
}


void mmd_scan_lines_vector(const char * text, size_t len, vector_line_node * v, uint32_t options) {
	// re2c scanner
	Scanner s = mmd_scanner(text, len);
	s.cur += skip_bom(text);

	vector_line_node_add(v, scanner_next_line_vector(v, &s, options));

	while (s.cur < s.stop) {
		vector_line_node_add(v, scanner_next_line_vector(v, &s, options));
	}
}


/// Given source text, parse it into AST
mmd_node * mmd_parse_text(const char * text, size_t len, vector_line_node * vl, mmd_node_pool * p, read_ctx * r, uint32_t options) {
	mmd_scan_lines_vector(text, len, vl, options);

	F(i, (int)vl->size) {
		vl->element[i].general.tail = (mmd_node *) &vl->element[i];

		if (i + 1 < (int)vl->size) {
			vl->element[i].general.next = (mmd_node *) &vl->element[i + 1];
		}
	}

	return doc((mmd_node *) &vl->element[0], p, text, len, r, options);
}


/// Given source text, return a tree of the line structure
/// Stop when metadata ends
static void mmd_scan_metadata_lines(const char * text, size_t len, vector_line_node * v, uint32_t options) {
	// re2c scanner
	Scanner s = mmd_scanner(text, len);
	s.cur += skip_bom(text);

	size_t old_size = 0;

	vector_line_node_add(v, scanner_next_line_vector(v, &s, options));

	while ((s.cur < s.stop) && (v->size > old_size) && (v->element[v->size - 1].general.type != LINE_EMPTY)) {
		vector_line_node_add(v, scanner_next_line_vector(v, &s, options));
		old_size++;
	}
}


mmd_node * mmd_parse_metadata(const char * text, size_t len, vector_line_node * vl, mmd_node_pool * p, read_ctx * r, uint32_t options) {
	if (!r->allow_meta) {
		return NULL;
	}

	mmd_scan_metadata_lines(text, len, vl, options);

	if (vl->size == 0) {
		return NULL;
	}

	F(i, (int)vl->size) {
		vl->element[i].general.tail = (mmd_node *) &vl->element[i];

		if (i + 1 < (int)vl->size) {
			vl->element[i].general.next = (mmd_node *) &vl->element[i + 1];
		}
	}

	mmd_line_node * l = &vl->element[0];

	mmd_node * m = NULL;

	switch (l->general.type) {
		case LINE_SETEXT_2:
			l->general.type = LINE_YAML;

		case LINE_YAML:
			m = block_meta_yaml((mmd_node **) &l, p);
			mmd_parse_meta_block(&text[m->start + m->child->len], m->len - m->child->len, r);

			{
				mmd_node * ll = (mmd_node *) l;

				// Add trailing YAML line (if present) *after* we parse the metadata block
				if (ll && (ll->type == LINE_SETEXT_2)) {
					ll->type = LINE_YAML;
					l = (mmd_line_node *) mmd_node_feed_chain(m, ll);
				}
			}

			break;

		case LINE_META:
			m = block_meta((mmd_node **) &l, p);
			mmd_parse_meta_block(&text[m->start], m->len, r);
			break;

		default:
			break;
	}

	if (m) {
		r->meta_end = m->start + m->len;
	}

	return m;
}


static mmd_node * mask_manual_label_token(mmd_node * b) {
	mmd_node * walker = b->content;
	int count = 0;
	mmd_node * candidate = NULL;

	while (walker) {
		switch (walker->type) {
			case TOKEN_PAIR_BRACKET:
				count++;

				if (count % 2) {
					candidate = walker;
				} else {
					candidate = NULL;
				}

				walker = walker->next;
				break;

			case TOKEN_ATX_MARKER:
			case TOKEN_NL:
			case TOKEN_TEXT_WHITESPACE:
				break;

			case TOKEN_TEXT:
				if (walker->len) {
					count = 0;
					candidate = NULL;
				}

				break;

			default:
				count = 0;
				candidate = NULL;
				break;
		}

		walker = walker->next;
	}

	if (candidate) {
		candidate->type = TOKEN_MANUAL_LABEL;
		candidate->next->type = TOKEN_MANUAL_LABEL;
	}

	return candidate;
}
