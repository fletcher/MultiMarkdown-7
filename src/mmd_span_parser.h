/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_span_parser.h

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


#ifndef MMD_SPAN_PARSER_LIBMULTIMARKDOWN7_H
#define MMD_SPAN_PARSER_LIBMULTIMARKDOWN7_H

#include <stdbool.h>

enum pairing_rules {
	PAIR_SKIP_IDENTICAL_TOKENS =		1 << 0,
	PAIR_MATCH_LENGTH =					1 << 1,
	PAIR_HAS_SUBTYPES =					1 << 2,
	PAIR_NO_MATCH_INTRAWORD =			1 << 3,
	PAIR_LIMIT_MULTIPLE_3 =				1 << 4,
	PAIR_NON_CONSECUTIVE =				1 << 5,


	PAIR_OPEN_NO_WS_LE_LEFT =			1 << 6,
	PAIR_OPEN_NO_PUNCT_LEFT =			1 << 7,
	PAIR_OPEN_NO_ALPHA_NUM_LEFT = 		1 << 8,
	PAIR_OPEN_IF_WS_LE_LEFT =			1 << 9,
	PAIR_OPEN_IF_PUNCT_LEFT =			1 << 10,

	PAIR_OPEN_NO_WS_LE_RIGHT =			1 << 11,
	PAIR_OPEN_NO_PUNCT_RIGHT =			1 << 12,
	PAIR_OPEN_NO_ALPHA_NUM_RIGHT = 		1 << 13,
	PAIR_OPEN_IF_WS_LE_RIGHT =			1 << 14,
	PAIR_OPEN_IF_PUNCT_RIGHT =			1 << 15,

	PAIR_CLOSE_NO_WS_LE_LEFT =			1 << 16,
	PAIR_CLOSE_NO_PUNCT_LEFT =			1 << 17,
	PAIR_CLOSE_NO_ALPHA_NUM_LEFT = 		1 << 18,
	PAIR_CLOSE_IF_WS_LE_LEFT =			1 << 19,
	PAIR_CLOSE_IF_PUNCT_LEFT =			1 << 20,

	PAIR_CLOSE_NO_WS_LE_RIGHT =			1 << 21,
	PAIR_CLOSE_NO_PUNCT_RIGHT =			1 << 22,
	PAIR_CLOSE_NO_ALPHA_NUM_RIGHT = 	1 << 23,
	PAIR_CLOSE_IF_WS_LE_RIGHT =			1 << 24,
	PAIR_CLOSE_IF_PUNCT_RIGHT =			1 << 25,



	// PAIR_CLOSE_IF_NON_WS_TO_LEFT =		1 << 0,
	// PAIR_OPEN_IF_NON_WS_TO_RIGHT =		1 << 1,
	// PAIR_OPEN_IF_NON_WS_PUNCT_TO_LEFT =	1 << 2,
	// PAIR_OPEN_IF_WS_TO_LEFT =			1 << 7,
	// PAIR_CLOSE_IF_WS_TO_RIGHT =			1 << 8,
	// PAIR_CLOSE_IF_WS_PUNCT_TO_RIGHT =	1 << 9,
};


typedef struct {
	enum pairing_rules 	conditions;
	unsigned char		mate_type;
	unsigned char		pairing_type;
} PairRule;


char * html_id_from_text(const char * text, size_t len, bool require_odd_count);
char * md_id_from_text(const char * text, size_t len, bool require_odd_count);
char * definition_name_from_text(const char * text, size_t len);

void mmd_parse_tokens_block(mmd_node * b, const char * text, read_ctx * c, mmd_node_pool * p, uint32_t options);
void mmd_parse_tokens_deflist(mmd_node * b, const char * text, read_ctx * c, mmd_node_pool * p, uint32_t options);
void mmd_parse_tokens_table(mmd_node * b, const char * text, read_ctx * c, mmd_node_pool * p, uint32_t options);

endnote_def * mmd_parse_tokens_endnote(mmd_node * b, const char * text, size_t len, read_ctx * c, mmd_node_pool * p, uint32_t options);
void mmd_parse_meta_block(const char * text, size_t len, read_ctx * c);

void analyze_token_chain(mmd_node_pool * p, mmd_node * n, PairRule pairings[], const char * text, read_ctx * c, uint32_t options);

#endif
