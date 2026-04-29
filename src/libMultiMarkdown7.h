/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file libMultiMarkdown7.h

	@brief

		If you are using libcurl, then you need to call the following once,
		to initialize the environment:

			curl_global_init(CURL_GLOBAL_ALL);

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



	MultiMarkdown 7 makes use of:

	uthash for hash tables
		https://troydhanson.github.io/uthash/

	miniz for zip archive handling
		https://github.com/richgel999/miniz

	yxml for XML parsing
		https://dev.yorhel.nl/yxml

	wingetopt for options parsing on Windows
		https://github.com/alex85k/wingetopt

	base64 for encoding binary data in HTML
		https://github.com/zhicheng/base64

	re2c is used to generate mmd_token_scanner.c and mmd_line_scanner.c
*/


#ifndef libMultiMarkdown7_H
#define libMultiMarkdown7_H

#include <stdint.h>
#include <stdio.h>


// Advance declarations
typedef struct mmd_node mmd_node;
typedef struct read_ctx read_ctx;
typedef struct stack stack;


/// Process MultiMarkdown text into another format
/// Output is sent to designated file stream (e.g. stdout)
/// search_path and source_path are used for file transclusion, can be nil to disable
void mmd_process_file(FILE * in, FILE * out, uint32_t options, const char * search_path, const char * source_path);
void mmd_process_filename(const char * fname, FILE * out, uint32_t options, const char * search_path);
void mmd_process_str(const char * text, FILE * out, uint32_t options, const char * search_path, const char * source_path);
void mmd_process_str_len(const char * text, size_t in_len, FILE * out, uint32_t options, const char * search_path, const char * source_path);
void mmd_process_url(const char * url, FILE * out, uint32_t options, const char * search_path, const char * source_path);


/// Returns text string (or binary data) -- will need to be freed
/// Length of output will be stored in out_len (especially needed for binary data formats)
char * mmd_process_file_to_str(FILE * in, size_t * out_len, uint32_t options, const char * search_path, const char * source_path);
char * mmd_process_filename_to_str(const char * fname, size_t * out_len, uint32_t options, const char * search_path);
char * mmd_process_str_to_str(const char * text, size_t * out_len, uint32_t options, const char * search_path, const char * source_path);
char * mmd_process_str_len_to_str(const char * text, size_t in_len, size_t * out_len, uint32_t options, const char * search_path, const char * source_path);
char * mmd_process_url_to_str(const char * url, size_t * out_len, uint32_t options, const char * search_path, const char * source_path);


/// Parse MultiMarkdown text into AST
/// Returns a tree of mmd_nodes that will need to be freed
/// read_ctx will be updated to contain structured information from the parse necessary
/// to create a full document (e.g. metadata, link definitions, etc.)
/// After parsing, you should not need to parse the original text again,
/// though you may have to grab certain character ranges
mmd_node * mmd_parse_file(FILE * in, read_ctx * c, uint32_t options);
mmd_node * mmd_parse_filename(const char * filename, read_ctx * c, uint32_t options);
mmd_node * mmd_parse_str(const char * text, read_ctx * c, uint32_t options);
mmd_node * mmd_parse_str_len(const char * text, size_t in_len, read_ctx * c, uint32_t options);


/// Process MultiMarkdown text into AST and output a description to
/// the specified file stream
void mmd_ast_file(FILE * in, FILE * out, uint32_t options);
void mmd_ast_filename(const char * fname, FILE * out, uint32_t options);
void mmd_ast_str(const char * text, FILE * out, uint32_t options);
void mmd_ast_str_len(const char * text, size_t in_len, FILE * out, uint32_t options);


/// Process MultiMarkdown text into AST with hash values and output a description to
/// the specified file stream
/// TODO: Add this to test suite for regression testing
void mmd_hash_file(FILE * in, FILE * out, uint32_t options);
void mmd_hash_filename(const char * fname, FILE * out, uint32_t options);
void mmd_hash_str(const char * text, FILE * out, uint32_t options);
void mmd_hash_str_len(const char * text, size_t in_len, FILE * out, uint32_t options);


/// Process MultiMarkdown text for metadata
/// read_ctx will need to be freed when finished
/// TODO: Make metadata extraction API simpler
read_ctx * mmd_metadata_filename(const char * fname, uint32_t options);
read_ctx * mmd_metadata_file(FILE * in, uint32_t options);
read_ctx * mmd_metadata_str(const char * text, uint32_t options);
read_ctx * mmd_metadata_str_len(const char * text, size_t in_len, uint32_t options);


/// Process MultiMarkdown text for tags
/// read_ctx will need to be freed when finished
read_ctx * mmd_tags_filename(const char * fname, uint32_t options);
read_ctx * mmd_tags_file(FILE * in, uint32_t options);
read_ctx * mmd_tags_str(const char * text, uint32_t options);
read_ctx * mmd_tags_str_len(const char * text, size_t in_len, uint32_t options);


/// Utility functions

void mmd_node_free(mmd_node * n);
void mmd_node_tree_free(mmd_node * n);

/// Calculate hash values for AST (and return overall hash value)
uint32_t mmd_hash_node_tree(mmd_node * n);
/// Calculate hash value for individual node (and it's children)
uint32_t mmd_hash_node(mmd_node * n);

/// Print node tree hash values to designated file stream
void mmd_node_tree_describe_hash(mmd_node * n, FILE * out);

/// read_ctx management
read_ctx * read_ctx_new(uint32_t options);
void read_ctx_reset(read_ctx * c, uint32_t options);
void read_ctx_free(read_ctx * c);

void custom_seed_rand(void);


enum output_format {
	FORMAT_HTML,								//!< Plain HTML
	FORMAT_AST,									//!< Display the AST for informational/debugging purposes
	FORMAT_EPUB,								//!< EPUB v3
	FORMAT_HASH,								//!< Display the AST with hash values for informational/debugging purposes
	FORMAT_ITMZ,								//!< iThoughts Mind Mapping document
	FORMAT_LATEX,								//!< LaTeX to generate PDF
	FORMAT_BEAMER,								//!< Beamer (slide presentations in LaTeX)
	FORMAT_LTX_TALK,							//!< ltx-talk (modern slide presentations in LaTeX)
	FORMAT_MMD,									//!< Raw MultiMarkdown source text
	FORMAT_OPML,								//!< Outline Processor Markup Language for outliners or mind-mapping programs
	FORMAT_TEXTBUNDLE,							//!< TextBundle is a package file format for macOS/iOS
	FORMAT_TEXTPACK,							//!< Compressed variant of the TextBundle file format

	FORMAT_DOCX,
	FORMAT_FODT,
	FORMAT_ODT,
};


enum smart_quote_language {
	QUOTES_ENGLISH,								//!< English smart quotes
	QUOTES_DUTCH,								//!< Dutch smart quotes
	QUOTES_FRENCH,								//!< French smart quotes
	QUOTES_GERMAN,								//!< German smart quotes
	QUOTES_GERMAN_GUILLEMETS,					//!< German guillemets smart quotes
	QUOTES_SPANISH,								//!< Spanish smart quotes
	QUOTES_SWEDISH,								//!< Swedish smart quotes
};


enum language {
	LANGUAGE_EN,								//!< English language markup
	LANGUAGE_ES,								//!< Spanish language markup
	LANGUAGE_DE,								//!< German language markup
	LANGUAGE_FR,								//!< French language markup
	LANGUAGE_NL,								//!< Dutch language markup
	LANGUAGE_SV,								//!< Swedish language markup
	LANGUAGE_HE,								//!< Hebrew language markup
};


enum mmd_options {
	// First 5 bits are for output_format (32 max)
	// Next 4 bits are for smart_quote_language (16 max)
	// Next 4 bits are for language (16 max)
	MMD_OPTION_TRANSCLUDE		= 1 << 13,		//!< Enable file transclusion
	MMD_OPTION_STATS			= 1 << 14,		//!< Display performance stats on stderr
	MMD_OPTION_BLOCKS_ONLY		= 1 << 15,		//!< Process block-level tokens only; do not parse inside the blocks
	MMD_OPTION_MMD_HEADER		= 1 << 16,		//!< Enable use of `mmd header` and `mmd footer` metadata
	MMD_OPTION_RANDOM_NOTE_ID	= 1 << 17,		//!< Use random footnote id # to avoid collisions
	MMD_OPTION_RANDOM_HEADER_ID	= 1 << 18,		//!< Use random header id # to avoid collisions
	MMD_OPTION_COMPATIBILITY	= 1 << 19,		//!< Limit functionality to core Markdown features
	MMD_OPTION_CRITIC_ACCEPT	= 1 << 20,		//!< Accept all proposed changes
	MMD_OPTION_CRITIC_REJECT	= 1 << 21,		//!< Reject all proposed changes
	MMD_OPTION_COMPLETE			= 1 << 22,		//!< Force creation of complete document
	MMD_OPTION_SNIPPET			= 1 << 23,		//!< Force creation of snippet instead of complete document
	MMD_OPTION_EMBED_ASSETS		= 1 << 24,		//!< Embed assets (images, CSS) within the output file itself (eg. HTML)
	MMD_OPTION_STORE_ASSETS		= 1 << 25,		//!< Store assets (images, CSS) within archive file formats
	MMD_OPTION_DOWNLOAD_ASSETS	= 1 << 26,		//!< Attempt to download assets from the internet for storage
	MMD_OPTION_PARSE_HTML		= 1 << 27,		//!< Convert from HTML to MMD text before parsing
	MMD_OPTION_PARSE_OPML		= 1 << 28,		//!< Convert from OPML to MMD text before parsing
	MMD_OPTION_PARSE_ITMZ		= 1 << 29,		//!< Convert from ITMZ to MMD text before parsing
};


/// Macros to extract specific options
#define MMD_OUT_FORMAT_MASK 0x1f
#define MMD_SMART_QUOTE_MASK 0x01e0
#define MMD_LANGUAGE_MASK 0x1E00

/// Extract Output format from options
#define MMD_OUT_FORMAT_FROM_OPTS(x) ((x & MMD_OUT_FORMAT_MASK) >> 0)

/// Extract Smart Quotes language from options
#define MMD_SMART_QUOTE_FROM_OPTS(x) ((x & MMD_SMART_QUOTE_MASK) >> 5)

/// Extract markup language from options
#define MMD_LANGUAGE_FROM_OPTS(x) ((x & MMD_LANGUAGE_MASK) >> 9)


/// Nodes are used to build the AST during parsing
struct mmd_node {
	unsigned char		type;		//!< type for this node
	uint32_t			hash;		//!< hash for the node, useful when comparing two parse trees for similar branches

	size_t				start;		//!< Starting offset (in bytes) in the source text for this node
	size_t				len;		//!< Byte length in the source text for this node

	struct mmd_node *	next;		//!< Pointer to next node in the AST
	struct mmd_node *	child;		//!< Pointer to first child node in the AST
	struct mmd_node *	tail;		//!< Pointer to last sibling node in the AST

	struct mmd_node *	content;	//!< If node was parsed into span-level content, place it here
};


/// Line nodes are used specifically for parsing individual lines of text into the block structure
struct mmd_line_node {
	mmd_node			general;	//!< mmd_line_node starts with regular mmd_node

	size_t				c_start;	//!< Starting offset (in bytes) for line content (excluding line level markup)
	size_t				c_len;		//!< Byte length for content of the line (excluding line level markup)
};

typedef struct mmd_line_node mmd_line_node;


/// Macros to determine node class based on type value
#define MMD_TYPE_MASK 0xc0
#define MMD_TOKEN_MASK 0x80

#define MMD_NODE_IS_LINE(x)		((((mmd_node*)x)->type & MMD_TYPE_MASK) == 0x00)
#define MMD_NODE_IS_BLOCK(x)	((((mmd_node*)x)->type & MMD_TYPE_MASK) == 0x40)
#define MMD_NODE_IS_TOKEN(x)	((((mmd_node*)x)->type & MMD_TOKEN_MASK) == 0x80)


/// AST node types
enum node_types {
	// Line types (1-63)
	LINE_ATX_1 = 1,
	LINE_ATX_2,
	LINE_ATX_3,
	LINE_ATX_4,
	LINE_ATX_5,
	LINE_ATX_6,
	LINE_BACKTICK,
	LINE_BLOCKQUOTE,
	LINE_CONTINUATION,
	LINE_DEF_ABBREVIATION,
	LINE_DEF_CITATION,
	LINE_DEF_FOOTNOTE,
	LINE_DEF_GLOSSARY,
	LINE_DEF_LINK,
	LINE_DEFINITION,
	LINE_EMPTY,
	LINE_FALLBACK,
	LINE_FENCE_BACKTICK_3,
	LINE_FENCE_BACKTICK_4,
	LINE_FENCE_BACKTICK_5,
	LINE_FENCE_BACKTICK_START_3,
	LINE_FENCE_BACKTICK_START_4,
	LINE_FENCE_BACKTICK_START_5,
	LINE_HR,
	LINE_HTML,
	LINE_HTML_BLOCK,
	LINE_HTML_BLOCKISH,
	LINE_INDENTED_SPACE,
	LINE_INDENTED_TAB,
	LINE_LIST_BULLETED,
	LINE_LIST_ENUMERATED,
	LINE_META,
	LINE_PLAIN,
	LINE_SETEXT_1,
	LINE_SETEXT_2,
	LINE_START_COMMENT,
	LINE_STOP_COMMENT,
	LINE_TABLE,
	LINE_TABLE_SEPARATOR,
	LINE_TOC,
	LINE_YAML,

	CODE_FENCE_LINE = 63,			// TODO: Do I really need to use this?


	// Block types (64-127)
	BLOCK_BLOCKQUOTE = 64,
	BLOCK_CODE_FENCED,
	BLOCK_CODE_INDENTED,
	BLOCK_DEF_ABBREVIATION,
	BLOCK_DEF_CITATION,
	BLOCK_DEF_FOOTNOTE,
	BLOCK_DEF_GLOSSARY,
	BLOCK_DEF_LINK,
	BLOCK_DEFINITION,
	BLOCK_DEFLIST,
	BLOCK_EMPTY,
	BLOCK_GENERAL,
	BLOCK_H1,
	BLOCK_H2,
	BLOCK_H3,
	BLOCK_H4,
	BLOCK_H5,
	BLOCK_H6,
	BLOCK_HEADING,
	BLOCK_HR,
	BLOCK_HTML,
	BLOCK_LIST_BULLETED,
	BLOCK_LIST_BULLETED_LOOSE,
	BLOCK_LIST_ENUMERATED,
	BLOCK_LIST_ENUMERATED_LOOSE,
	BLOCK_LIST_ITEM,
	BLOCK_LIST_ITEM_TIGHT,
	BLOCK_META,
	BLOCK_PARA,
	BLOCK_SETEXT_1,
	BLOCK_SETEXT_2,
	BLOCK_TABLE,
	BLOCK_TABLE_HEADER,
	BLOCK_TABLE_SECTION,
	BLOCK_TABLE_ROW,
	BLOCK_TABLE_SEPARATOR,
	BLOCK_TERM,
	BLOCK_TOC,
	BLOCK_FIGURE,


	// Token types (128-255)
	TOKEN_EOF = 128,
	TOKEN_NL,
	TOKEN_LINEBREAK,
	TOKEN_TEXT,
	TOKEN_TEXT_ABBREVIATION,
	TOKEN_TEXT_GLOSSARY,
	TOKEN_TEXT_WHITESPACE,

	TOKEN_AMPERSAND,
	TOKEN_AMPERSAND_LONG,
	TOKEN_HTML_ENTITY,

	TOKEN_HASH,

	TOKEN_STAR,
	TOKEN_PLUS,
	TOKEN_MINUS,

	TEXT_NUMBER_POSS_LIST,

	TOKEN_UL,
	TOKEN_COLON,

	TOKEN_ATX_MARKER,
	TOKEN_BLOCKQUOTE_MARKER,
	TOKEN_DEFLIST_COLON,
	TOKEN_LIST_MARKER,
	TOKEN_ABBREVIATION_MARKER,
	TOKEN_FOOTNOTE_MARKER,
	TOKEN_GLOSSARY_MARKER,
	TOKEN_CITATION_MARKER,
	TOKEN_VARIABLE_MARKER,

	TOKEN_BACKTICK,
	TOKEN_APOSTROPHE,
	TOKEN_QUOTE_SINGLE,
	TOKEN_QUOTE_DOUBLE,
	TOKEN_QUOTE_DOUBLE_ALT,
	TOKEN_ELLIPSIS,
	TOKEN_DASH_M,
	TOKEN_DASH_N,
	TOKEN_DASH_N_RANGE,

	TOKEN_PAREN_LEFT,
	TOKEN_PAREN_RIGHT,
	TOKEN_BRACKET_LEFT,
	TOKEN_BRACKET_RIGHT,
	TOKEN_ANGLE_LEFT,
	TOKEN_ANGLE_RIGHT,
	TOKEN_BRACE_LEFT,
	TOKEN_BRACE_RIGHT,

	TOKEN_PAIR_ANGLE,
	TOKEN_PAIR_BACKTICK,
	TOKEN_PAIR_BRACE,
	TOKEN_PAIR_BRACKET,
	TOKEN_PAIR_BRACKET_EMPTY,
	TOKEN_PAIR_BRACKET_NOT_CITED,
	TOKEN_PAIR_BRACKET_ABBREVIATION,
	TOKEN_PAIR_BRACKET_FOOTNOTE,
	TOKEN_PAIR_BRACKET_GLOSSARY,
	TOKEN_PAIR_BRACKET_CITATION,
	TOKEN_PAIR_BRACKET_IMAGE,
	TOKEN_PAIR_BRACKET_LINK,
	TOKEN_PAIR_BRACKET_VARIABLE,
	TOKEN_PAIR_PAREN,
	TOKEN_PAIR_QUOTE_DOUBLE,
	TOKEN_PAIR_QUOTE_SINGLE,
	TOKEN_PAIR_STAR,
	TOKEN_PAIR_STAR_USED,			//!< Must immediately follow TOKEN_PAIR_STAR
	TOKEN_PAIR_UL,
	TOKEN_PAIR_UL_USED,				//!< Must immediately follow TOKEN_PAIR_UL
	TOKEN_SPECIAL_CHARACTER,

	TOKEN_PAIR_EMPH,
	TOKEN_PAIR_STRONG,

	TOKEN_ESCAPED_CHARACTER,
	TOKEN_NBSP,
	TOKEN_PIPE,

	TOKEN_CM_ADD_OPEN,
	TOKEN_CM_ADD_CLOSE,
	TOKEN_CM_DEL_OPEN,
	TOKEN_CM_DEL_CLOSE,
	TOKEN_CM_SUB_OPEN,
	TOKEN_CM_SUB_DIV,
	TOKEN_CM_SUB_CLOSE,
	TOKEN_CM_COM_OPEN,
	TOKEN_CM_COM_CLOSE,
	TOKEN_CM_HI_OPEN,
	TOKEN_CM_HI_CLOSE,

	TOKEN_PAIR_CM_ADD,
	TOKEN_PAIR_CM_DEL,
	TOKEN_PAIR_CM_SUB_DEL,
	TOKEN_PAIR_CM_SUB_ADD,
	TOKEN_PAIR_CM_COM,
	TOKEN_PAIR_CM_HI,

	TOKEN_SUPERSCRIPT,
	TOKEN_SUBSCRIPT,

	TOKEN_PAIR_SUPERSCRIPT,
	TOKEN_PAIR_SUBSCRIPT,

	TOKEN_MATH_PAREN_OPEN,
	TOKEN_MATH_PAREN_CLOSE,
	TOKEN_MATH_BRACKET_OPEN,
	TOKEN_MATH_BRACKET_CLOSE,
	TOKEN_MATH_DOLLAR_SINGLE,
	TOKEN_MATH_DOLLAR_DOUBLE,

	TOKEN_PAIR_MATH_PAREN,
	TOKEN_PAIR_MATH_BRACKET,
	TOKEN_PAIR_MATH_DOLLAR_SINGLE,
	TOKEN_PAIR_MATH_DOLLAR_DOUBLE,

	TOKEN_TABLE_CELL,
	TOKEN_TABLE_DIVIDER,

	TOKEN_MANUAL_LABEL,

	TOKEN_TAG,

	OBJECT_REPLACEMENT_CHARACTER,   // This must be the last type
};


void read_ctx_dump_headers(read_ctx * c);

#endif
