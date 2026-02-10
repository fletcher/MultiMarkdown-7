/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file export_core.h

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


#ifndef EXPORT_CORE_LIBMULTIMARKDOWN7_H
#define EXPORT_CORE_LIBMULTIMARKDOWN7_H

enum descent {
	VERBATIM				= 0,
	DESCEND_CHILD			= 1 << 0,
	DESCEND_CONTENT			= 1 << 1,
	DESCEND_LINES			= 1 << 2,
	DESCEND_LINES_RAW		= 1 << 3,
	DESCEND_LINES_VERBATIM	= 1 << 4,
	DESCEND_RAW				= 1 << 5,
	DESCEND_VERBATIM		= 1 << 6,
	NONE					= 1 << 7,
};


typedef struct {
	char			padding;

	const char *	prefix;
	int 			pre_descent_padding;
	int				descent;
	int 			pad_post_descent;
	const char *	suffix;
	int				post_suffix_padding;
	int				skip;

	// The following are calculated with precalculate_rules(), so do not need to be included manually in the code
	int 			prefix_len;
	int				suffix_len;
} parse_rule;


typedef struct {
	const char *	opener;
	const char *	closer;

	// The following are calculated with precalculate_rules(), so do not need to be included manually in the code
	int				opener_len;
	int				closer_len;
} smart_quote;


typedef struct {
	void			(* export_raw_text)(const char * text, size_t len, text_buffer * out);
	void			(* export_tokens)(mmd_node * t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options);

	int				(* export_link_def)(link_def * l, const char * text, size_t len, mmd_node * n, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options);
	int				(* export_link_def_image)(link_def * l, const char * text, size_t len, mmd_node * n, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options, bool is_figure);
	int				(* export_abbr_def)(abbr_def * a, const char * text, size_t len, mmd_node * key_token, mmd_node * expansion_token, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options, bool * first);
	int				(* export_endnote_def)(unsigned char type, endnote_def * e, const char * text, size_t len, mmd_node * n, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options);
} format_export;


#define mmd_print_const(x, y) text_buffer_append_text(x, y, sizeof(y) - 1)

char * mmd_strndup(const char * source, size_t * n);

void precalculate_rules(parse_rule * rules, int n);
void precalculate_quotes(smart_quote * quotes, int n);

link_def * extract_inline_link(const char * text, size_t len, mmd_node ** t, uint32_t options);

void url_encode_text(const char * text, size_t len, text_buffer * out);

/// Interpret meaning of a [...] based on next token and content (e.g. link, image, abbreviation, citation, footnote, glossary, variable)
int export_token_pair(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options);

int raw_level_for_header(mmd_node * n);

int raw_filter_matches_format(const char * pattern, int format);

#endif
