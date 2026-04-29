/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file read_ctx.h

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


#ifndef READ_CTX_LIBMULTIMARKDOWN7_H
#define READ_CTX_LIBMULTIMARKDOWN7_H

#include <stdbool.h>
#include "uthash.h"

#include "stack.h"


/// Extracted headers
typedef struct {
	char		*		key;

	mmd_node	*		node;

	const char *		text;
	size_t				text_len;

	size_t				c_start;
	size_t				c_len;
} header;


/// Extracted metadata
typedef struct {
	char 		*		key;

	char 		*		value;
	size_t				value_start;
	size_t				value_len;

	UT_hash_handle		hh;
} meta;


/// Extracted tags
typedef struct {
	char *				key;

	UT_hash_handle		hh;
} tag;


/// Extracted link reference definitions
struct attr {
	char *				key;
	char *				value;
	struct attr *		next;
};

typedef struct attr attr;

typedef struct {
	char 		*		key;

	char 		*		url;
	size_t				url_len;

	char *				title;
	size_t				title_len;

	attr *				attributes;

	UT_hash_handle		hh;
} link_def;


/// Extracted abbreviation reference definitions
typedef struct {
	char 		*		key;

	char 		*		expansion;
	size_t				expansion_len;

	bool				used;

	UT_hash_handle		hh;
} abbr_def;


/// Extracted endnote reference definitions (footnote, citation, glossary)
typedef struct {
	char 		*		key;
	mmd_node *			key_node;

	bool 				is_inline;

	int					index;

	const char 	*		text;
	size_t				len;

	mmd_node 	*		title_node;		//!< Only used in glossary entries
	mmd_node 	*		content_node;

	bool 				used;

	bool 				citet;			//!< Only used for citation
	bool 				not_cited;		//!< Only used for citation

	UT_hash_handle		hh;
} endnote_def;


enum media_type {
	textCSS,
	imagePNG,
	imageJPEG,
};


/// Assets (e.g. files that should be stored in zipfile formats -- images, CSS)
struct asset {
	char *					url;
	char *					uuid;
	char 					stored;
	enum media_type 		type;
	void *					data;
	size_t					len;
	struct UT_hash_handle	hh;
};

typedef struct asset asset;


/// Structured information from parsing process
struct read_ctx {
	char				allow_meta;
	char				has_meta;
	size_t				meta_end;

	char				write_complete;
	char				write_snippet;

	int					base_header_level;
	int					epub_header_level;
	int					html_header_level;
	int					latex_header_level;
	int					beamer_header_level;

	char				language;
	char				quotes_language;

	char				is_blockquote;
	char				is_definition;
	char				is_endnote;
	char				is_list_item;

	stack 	*			token_pair_stack;

	stack	*			header_stack;
	uint16_t			random_header_seed;

	meta 	*			meta_hash;
	tag		*			tag_hash;
	link_def 	*		link_def_hash;
	abbr_def 	*		abbr_def_hash;

	endnote_def *		cite_def_hash;
	endnote_def *		glos_def_hash;
	endnote_def *		note_def_hash;

	int					cite_used;
	int					glos_used;
	int					note_used;

	asset *				asset_hash;
};


#define MMD_HEADER_LEVEL_DISABLED -6


read_ctx * read_ctx_new(uint32_t options);
void read_ctx_reset(read_ctx * c, uint32_t options);
void read_ctx_free(read_ctx * c);

void read_ctx_store_internal_link(read_ctx * c, const char * text, size_t len, bool require_odd_count);
void read_ctx_store_internal_link_key(read_ctx * c, const char * key, size_t key_len);
void read_ctx_store_header(read_ctx * c, const char * text, size_t len, mmd_node * n, const char * key, size_t key_len, size_t c_start, size_t c_len);

void read_ctx_store_abbr(read_ctx * c, abbr_def * l);
void read_ctx_store_link(read_ctx * c, link_def * l);
void read_ctx_store_meta(read_ctx * c, meta * m);
void read_ctx_store_tag(read_ctx * c, const char * tag, size_t tag_len);

int read_ctx_store_cite(read_ctx * c, endnote_def * e);
int read_ctx_store_glos(read_ctx * c, endnote_def * e);
int read_ctx_store_note(read_ctx * c, endnote_def * e);


abbr_def * read_ctx_get_abbr(read_ctx * c, char * key);
link_def * read_ctx_get_link(read_ctx * c, char * key);
meta * read_ctx_get_meta(read_ctx * c, char * key);
tag * read_ctx_get_tag(read_ctx * c, char * key);

endnote_def * read_ctx_get_cite(read_ctx * c, char * key);
endnote_def * read_ctx_get_glos(read_ctx * c, char * key);
endnote_def * read_ctx_get_note(read_ctx * c, char * key);

int read_ctx_get_header_level(read_ctx * c, int format);

void header_free(header * h);
void meta_free(meta * m);
void link_def_free(link_def * l);
void abbr_def_free(abbr_def * a);
void endnote_def_free(endnote_def * e);

asset * read_ctx_get_asset(read_ctx * c, char * url);
asset * read_ctx_store_asset(read_ctx * c, char * url, size_t url_len, uint32_t options, const char * source_path);

#endif
