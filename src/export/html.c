/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file html.c

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
#include "il8n.h"
#include "mmd_scanner.h"
#include "mmd_token_scanner.h"
#include "mmd_utilities.h"

#include "export_core.h"
#include "html.h"

#include "base64.h"

// #include <threads.h> // The header <threads.h> defines thread_local as a synonym for _Thread_local
//thread_local const char * g_search_path = NULL;

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	__declspec(thread) char * g_search_path = NULL;
#else
	__thread char * g_search_path = NULL;
#endif


#ifdef TEST
	#include "CuTest.h"
#endif

#define F(i,n) for(int i= 0;i<n;i++)

static void export_html_blocks(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options);


static char * media_type_string[] = {
	[textCSS] = "text/css",
	[imagePNG] = "image/png",
	[imageJPEG] = "image/jpeg",
};


static parse_rule rules[256] = {
	[BLOCK_BLOCKQUOTE]				= { 2, "<blockquote>", 1, DESCEND_CHILD, 1, "</blockquote>", 0, 0, 0, 0 },
	[BLOCK_CODE_INDENTED]			= { 2, "<pre><code>", 0, DESCEND_LINES_RAW, 0, "</code></pre>", 0, 0, 0, 0 },
	[BLOCK_FIGURE]					= { 2, NULL, 0, DESCEND_CONTENT, 0, NULL, 0, 0, 0, 0 },
	[BLOCK_HR]						= { 2, "<hr />", 0, 0, 0, NULL, 0, 0, 0, 0 },
	[BLOCK_HTML]					= { 2, NULL, 0, DESCEND_LINES, 0, NULL, 0, 0, 0, 0 },
	[BLOCK_LIST_BULLETED]			= { 2, "<ul>", 0, DESCEND_CHILD, 1, "</ul>", 0, 0, 0, 0 },
	[BLOCK_LIST_BULLETED_LOOSE]		= { 2, "<ul>", 0, DESCEND_CHILD, 1, "</ul>", 0, 0, 0, 0 },
	[BLOCK_LIST_ENUMERATED]			= { 2, "<ol>", 0, DESCEND_CHILD, 1, "</ol>", 0, 0, 0, 0 },
	[BLOCK_LIST_ENUMERATED_LOOSE]	= { 2, "<ol>", 0, DESCEND_CHILD, 1, "</ol>", 0, 0, 0, 0 },
	[BLOCK_LIST_ITEM]				= { 1, "<li>", 2, DESCEND_CHILD, 0, "</li>", 0, 0, 0, 0 },
	[BLOCK_PARA]					= { 2, "<p>", 0, DESCEND_CONTENT, 0, "</p>", 0, 0, 0, 0 },
	[BLOCK_TERM]					= { 2, "<dt>", 0, DESCEND_CONTENT, 0, "</dt>\n", 2, 0, 0, 0 },

	[BLOCK_TABLE_HEADER]			= { 1, "<thead>", 0, DESCEND_CHILD, 1, "</thead>", 0, 0, 0, 0 },
	[BLOCK_TABLE_SECTION]			= { 2, "<tbody>", 0, DESCEND_CHILD, 1, "</tbody>", 0, 0, 0, 0 },
	[BLOCK_TABLE_ROW]				= { 1, "<tr>\n", 1, DESCEND_CONTENT, 1, "</tr>", 0, 0, 0, 0 },

	[TOKEN_PAIR_CM_SUB_ADD]			= { 0, "~&gt;", 0, DESCEND_CHILD, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_PAIR_CM_ADD]				= { 0, "<ins>", 0, DESCEND_CHILD, 0, "</ins>", 0, 1, 0, 0 },
	[TOKEN_PAIR_CM_DEL]				= { 0, "<del>", 0, DESCEND_CHILD, 0, "</del>", 0, 1, 0, 0 },
	[TOKEN_PAIR_CM_COM]				= { 0, "<span class=\"critic comment\">", 0, DESCEND_CHILD, 0, "</span>", 0, 1, 0, 0 },
	[TOKEN_PAIR_CM_HI]				= { 0, "<mark>", 0, DESCEND_CHILD, 0, "</mark>", 0, 1, 0, 0 },

	[TOKEN_PAIR_BACKTICK]			= { 0, "<code>", 0, DESCEND_RAW, 0, "</code>", 0, 1, 0, 0 },
	[TOKEN_PAIR_EMPH]				= { 0, "<em>", 0, DESCEND_CHILD, 0, "</em>", 0, 1, 0, 0 },
	[TOKEN_PAIR_MATH_PAREN]			= { 0, "<span class=\"math\">\\(", 0, DESCEND_RAW, 0, "\\)</span>", 0, 1, 0, 0 },
	[TOKEN_PAIR_MATH_BRACKET]		= { 0, "<span class=\"math\">\\[", 0, DESCEND_RAW, 0, "\\]</span>", 0, 1, 0, 0 },
	[TOKEN_PAIR_MATH_DOLLAR_SINGLE]	= { 0, "<span class=\"math\">\\(", 0, DESCEND_RAW, 0, "\\)</span>", 0, 1, 0, 0 },
	[TOKEN_PAIR_MATH_DOLLAR_DOUBLE]	= { 0, "<span class=\"math\">\\[", 0, DESCEND_RAW, 0, "\\]</span>", 0, 1, 0, 0 },
	[TOKEN_PAIR_STRONG]				= { 0, "<strong>", 0, DESCEND_CHILD, 0, "</strong>", 0, 1, 0, 0 },
	[TOKEN_PAIR_STAR]				= { 0, "*", 0, DESCEND_CHILD, 0, "*", 0, 1, 0, 0 },
	[TOKEN_PAIR_STAR_USED]			= { 0, NULL, 0, DESCEND_CHILD, 0, NULL, 0, 1, 0, 0 },
	[TOKEN_PAIR_UL]					= { 0, "_", 0, DESCEND_CHILD, 0, "_", 0, 1, 0, 0 },
	[TOKEN_PAIR_UL_USED]			= { 0, NULL, 0, DESCEND_CHILD, 0, NULL, 0, 1, 0, 0 },
	[TOKEN_PAIR_PAREN]				= { 0, "(", 0, DESCEND_CHILD, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_PAIR_BRACE]				= { 0, "{", 0, DESCEND_CHILD, 0, NULL, 0, 0, 0, 0 },

	[TOKEN_ANGLE_LEFT]				= { 0, "&lt;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_ANGLE_RIGHT]				= { 0, "&gt;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_APOSTROPHE]				= { 0, "&#8217;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_DASH_M]					= { 0, "&#8212;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_DASH_N]					= { 0, "&#8211;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_ELLIPSIS]				= { 0, "&#8230;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_AMPERSAND]				= { 0, "&amp;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_AMPERSAND_LONG]			= { 0, "&amp;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_NBSP]					= { 0, "&nbsp;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_QUOTE_DOUBLE]			= { 0, "&quot;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_TEXT_WHITESPACE]			= { 0, " ", 0, NONE, 0, NULL, 0, 0, 0, 0 },

	[TOKEN_CM_SUB_DIV]				= { 0, "~&gt;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_CM_COM_OPEN]				= { 0, "{&gt;&gt;", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_CM_COM_CLOSE]			= { 0, "&lt;&lt;}", 0, NONE, 0, NULL, 0, 0, 0, 0 },
};


static smart_quote single_quotes[16] = {
	[QUOTES_ENGLISH]			= { "&#8216;", "&#8217;", 0, 0 },
	[QUOTES_DUTCH]				= { "&#8216;", "&#8217;", 0, 0 },
	[QUOTES_SWEDISH]			= { "&#8217;", "&#8217;", 0, 0 },
	[QUOTES_FRENCH]				= { "&#39;", "&#8217;", 0, 0 },
	[QUOTES_GERMAN]				= { "&#8218;", "&#8216;", 0, 0 },
	[QUOTES_GERMAN_GUILLEMETS]	= { "&#8250;", "&#8249;", 0, 0 },
	[QUOTES_SPANISH]			= { "&#8216;", "&#8217;", 0, 0 },
};


static smart_quote double_quotes[16] = {
	[QUOTES_ENGLISH]			= { "&#8220;", "&#8221;", 0, 0 },
	[QUOTES_DUTCH]				= { "&#8222;", "&#8221;", 0, 0 },
	[QUOTES_SWEDISH]			= { "&#8221;", "&#8221;", 0, 0 },
	[QUOTES_FRENCH]				= { "&#171;", "&#187;", 0, 0 },
	[QUOTES_GERMAN]				= { "&#8222;", "&#8220;", 0, 0 },
	[QUOTES_GERMAN_GUILLEMETS]	= { "&#187;", "&#171;", 0, 0 },
	[QUOTES_SPANISH]			= { "&#171;", "&#187;", 0, 0 },
};


static smart_quote headers_compat[6] = {
	{ "<h1>", "</h1>", 0, 0  },
	{ "<h2>", "</h2>", 0, 0  },
	{ "<h3>", "</h3>", 0, 0  },
	{ "<h4>", "</h4>", 0, 0  },
	{ "<h5>", "</h5>", 0, 0  },
	{ "<h6>", "</h6>", 0, 0  },
};

static smart_quote headers[6] = {
	{ "<h1 id=\"", "</h1>", 0, 0  },
	{ "<h2 id=\"", "</h2>", 0, 0  },
	{ "<h3 id=\"", "</h3>", 0, 0  },
	{ "<h4 id=\"", "</h4>", 0, 0  },
	{ "<h5 id=\"", "</h5>", 0, 0  },
	{ "<h6 id=\"", "</h6>", 0, 0  },
};


/// Print each line as is
static void export_html_line(mmd_node * n, const char * text, text_buffer * out) {
	switch (n->type) {
		case LINE_SETEXT_1:
		case LINE_SETEXT_2:
			break;

		default:
			if (n->next) {
				text_buffer_append_text(out, &text[n->start], (int)n->len);
				text_buffer_fix_trailing_newline(out);
			} else {
				// Don't print final trailing newline
				text_buffer_append_text(out, &text[n->start], (int)n->len - 1);

				// On Windows it might be messier, so clean up anything left over
				text_buffer_trim_trailing_newline(out);
			}

			break;
	}
}


static void export_html_lines(mmd_node * n, const char * text, text_buffer * out) {
	while (n) {
		export_html_line(n, text, out);

		n = n->next;
	}
}


/// Print each line as is, but only the "content" portion of each line
static void export_html_line_content(mmd_line_node * l, const char * text, text_buffer * out) {
	text += l->general.start + l->c_start;

	switch (l->general.type) {
		case LINE_SETEXT_1:
		case LINE_SETEXT_2:
			break;

		case LINE_EMPTY:
			text_buffer_append_c(out, '\n');
			break;

		default:
			if (l->general.next) {
				text_buffer_append_text(out, text, (int)l->c_len);
				text_buffer_fix_trailing_newline(out);
			} else if (l->c_len > 1) {
				// Don't print final trailing newline
				text_buffer_append_text(out, text, (int)l->c_len - 1);

				// On Windows it might be messier, so clean up anything left over
				text_buffer_trim_trailing_newline(out);
			}

			break;
	}
}


static void export_html_lines_content(mmd_node * n, const char * text, text_buffer * out) {
	while (n) {
		export_html_line_content((mmd_line_node *) n, text, out);

		n = n->next;
	}
}


/// Each output format has certain characters that must be escaped in various ways
static void export_html_raw_char(char c, text_buffer * out) {
	switch (c) {
		case '&':
			mmd_print_const(out, "&amp;");
			break;

		case '<':
			mmd_print_const(out, "&lt;");
			break;

		case '>':
			mmd_print_const(out, "&gt;");
			break;

		case '"':
			mmd_print_const(out, "&quot;");
			break;

		case '\r':
			break;

		default:
			text_buffer_append_c(out, c);
			break;
	}
}


static void export_html_raw_text(const char * text, size_t len, text_buffer * out) {
	const char * stop = text + len;

	while (text < stop) {
		export_html_raw_char(*text, out);

		text++;
	}
}


#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
/// Write text as-is, except for \r characters
static void export_text_except_cr(const char * text, size_t len, text_buffer * out) {
	const char * stop = text + len;

	while (text < stop) {
		switch (*text) {
			case '\r':
				break;

			default:
				text_buffer_append_c(out, *text);
				break;
		}

		text++;
	}
}
#endif


static void export_html_tokens_raw(mmd_node * t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w);

static void export_html_token_raw(mmd_node ** t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w) {
	switch ((*t)->type) {
		case TOKEN_PAIR_EMPH:
		case TOKEN_PAIR_STRONG:
		case TOKEN_PAIR_STAR:
		case TOKEN_PAIR_STAR_USED:
			export_html_tokens_raw((*t)->child, text, len, out, r, w);
			(*t) = (*t)->next;
			break;

		default:
			export_html_raw_text(&text[(*t)->start], (*t)->len, out);
			break;
	}
}


static void export_html_tokens_raw(mmd_node * t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w) {
	while (t) {
		export_html_token_raw(&t, text, len, out, r, w);
		t = t->next;
	}
}


static void export_html_line_raw(mmd_line_node * l, const char * text, text_buffer * out) {
	text += l->general.start;

	switch (l->general.type) {
		case CODE_FENCE_LINE:
			break;

		case LINE_EMPTY:
			text_buffer_append_c(out, '\n');
			break;

		default:
			export_html_raw_text(text, l->general.len, out);
			break;
	}
}


static void export_html_lines_raw(mmd_line_node * l, const char * text, text_buffer * out) {
	while (l) {
		export_html_line_raw(l, text, out);

		l = (mmd_line_node *) l->general.next;
	}
}


static void export_html_line_raw_content(mmd_line_node * l, const char * text, text_buffer * out) {
	text += l->general.start + l->c_start;

	switch (l->general.type) {
		case CODE_FENCE_LINE:
			break;

		case LINE_EMPTY:
			text_buffer_append_c(out, '\n');
			break;

		default:
			export_html_raw_text(text, l->c_len, out);
			break;
	}
}


static void export_html_lines_raw_content(mmd_line_node * l, const char * text, text_buffer * out) {
	while (l) {
		export_html_line_raw_content(l, text, out);

		l = (mmd_line_node *) l->general.next;
	}
}


static void export_toc_entry(text_buffer * out, size_t * counter, int level, int min, int max, read_ctx * r, write_ctx * w, uint32_t options) {
	header * h, * next;
	int h_level, next_level;

	mmd_print_const(out, "\n<ul>\n");

	while (*counter < r->header_stack->size) {
		h = stack_peek_index(r->header_stack, *counter);
		h_level = raw_level_for_header(h->node);

		if (h_level < min || h_level > max) {
			// Ignore this header
		} else {
			if (h_level >= level) {
				// This header is a direct descendant of the parent
				text_buffer_append_printf(out, "<li><a href=\"#%s\">", h->key);
				export_html_tokens(h->node->content, h->text, h->text_len, out, r, w, options);
				text_buffer_trim_trailing_whitespace(out);
				mmd_print_const(out, "</a>");

				if (*counter < r->header_stack->size - 1) {
					next = stack_peek_index(r->header_stack, *counter + 1);
					next_level = raw_level_for_header(next->node);

					if (next_level > h_level) {
						// This entry has children
						(*counter)++;
						export_toc_entry(out, counter, h_level + 1, min, max, r, w, options);
					}
				}

				mmd_print_const(out, "</li>\n");
			} else if (h_level < level) {
				// Decrement counter and exit this level
				(*counter)--;
				break;
			}
		}

		// Increment counter
		(*counter)++;
	}

	mmd_print_const(out, "</ul>\n");
}


static void export_html_toc(const char * text, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	size_t counter = 0;
	int min = 0;
	int max = 6;

	scan_toc_range(text, &min, &max);

	export_toc_entry(out, &counter, 0, min, max, r, w, options);
}


/// Core function to export link_def to HTML
static int export_html_link_def(link_def * l, const char * link_text, size_t link_text_len, mmd_node * link_text_token, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	if (l) {
		mmd_print_const(out, "<a href=\"");
		url_encode_text(l->url, l->url_len, out);

		if (l->title_len) {
			mmd_print_const(out, "\" title=\"");
			export_html_raw_text(l->title, l->title_len, out);
		}

		attr * a = l->attributes;

		while (a) {
			text_buffer_append_printf(out, "\" %s=\"%s", a->key, a->value);
			a = a->next;
		}

		mmd_print_const(out, "\">");
		export_html_tokens(link_text_token, link_text, link_text_len, out, r, w, options);
		mmd_print_const(out, "</a>");

		return 0;
	}

	return 1;
}


static int export_link_def_image(link_def * l, const char * link_text, size_t link_text_len, mmd_node * link_text_token, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options, bool is_figure) {
	if (!l) {
		return 1;
	}

	if (is_figure) {
		mmd_print_const(out, "<figure>\n");
	}

	mmd_print_const(out, "<img src=\"");

	if (options & MMD_OPTION_EMBED_ASSETS) {
		asset * a = read_ctx_store_asset(r, l->url, l->url_len, options, g_search_path);

		if (a && a->len > 0) {
			// Embed image binary data directly (Base64 encoded)
			text_buffer_append_printf(out, "data:%s;base64,", media_type_string[a->type]);
			unsigned int b64_len = BASE64_ENCODE_OUT_SIZE(a->len);
			char * b64 = malloc(b64_len);
			b64_len = base64_encode(a->data, (int)a->len, b64);
			text_buffer_append_text(out, b64, b64_len);
			free(b64);
		} else {
			url_encode_text(l->url, l->url_len, out);
		}
	} else if (options & MMD_OPTION_STORE_ASSETS) {
		asset * a = read_ctx_store_asset(r, l->url, l->url_len, options, g_search_path);

		if (a) {
			mmd_print_const(out, "assets/");
			text_buffer_append_text(out, a->uuid, 36);
		} else {
			url_encode_text(l->url, l->url_len, out);
		}
	} else {
		url_encode_text(l->url, l->url_len, out);
	}

	if (link_text_token) {
		mmd_print_const(out, "\" alt=\"");
		export_html_tokens_raw(link_text_token, link_text, link_text_len, out, r, w);
	}

	if (!(options & MMD_OPTION_COMPATIBILITY) && l->key) {
		mmd_print_const(out, "\" id=\"");
		// Need html_id instead of md_id
		char * id = html_id_from_text(l->key, strlen(l->key), false);
		export_html_raw_text(id, strlen(id), out);
		free(id);
	}

	if (l->title_len) {
		mmd_print_const(out, "\" title=\"");
		export_html_raw_text(l->title, l->title_len, out);
	}

	attr * a = l->attributes;

	while (a) {
		if (!strncmp("width", a->key, 5)) {
			if (!strncmp("px", &a->value[strlen(a->value) - 2], 2)) {
				text_buffer_append_printf(out, "\" %s=\"%.*s", a->key, strlen(a->value) - 2, a->value);
			} else {
				text_buffer_append_printf(out, "\" %s=\"%s", a->key, a->value);
			}
		} else if (!strncmp("height", a->key, 6)) {
			if (!strncmp("px", &a->value[strlen(a->value) - 2], 2)) {
				text_buffer_append_printf(out, "\" %s=\"%.*s", a->key, strlen(a->value) - 2, a->value);
			} else {
				text_buffer_append_printf(out, "\" %s=\"%s", a->key, a->value);
			}
		} else {
			text_buffer_append_printf(out, "\" %s=\"%s", a->key, a->value);
		}

		a = a->next;
	}

	mmd_print_const(out, "\" />");

	if (is_figure) {
		if (link_text_token) {
			mmd_print_const(out, "\n<figcaption>");
			export_html_tokens(link_text_token, link_text, link_text_len, out, r, w, options);
			mmd_print_const(out, "</figcaption>\n</figure>");
		} else {
			mmd_print_const(out, "\n</figure>");
		}
	}

	return 0;
}


static int export_abbr_def(abbr_def * a, const char * link_text, size_t link_text_len, mmd_node * key_token, mmd_node * expansion_token, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options, bool * first) {
	if (a) {
		if (a->used) {
			mmd_print_const(out, "<abbr title=\"");
		} else {
			if (expansion_token) {
				export_html_tokens(expansion_token, link_text, link_text_len, out, r, w, options);
			} else {
				export_html_raw_text(a->expansion, a->expansion_len, out);
			}

			mmd_print_const(out, " <abbr title=\"");
		}

		export_html_raw_text(a->expansion, a->expansion_len, out);

		if (a->used) {
			mmd_print_const(out, "\">");
		} else {
			mmd_print_const(out, "\">(");
		}

		if (key_token) {
			export_html_tokens(key_token, link_text, link_text_len, out, r, w, options);
		} else {
			export_html_raw_text(&a->key[1], strlen(a->key) - 1, out);
		}

		if (a->used) {
			mmd_print_const(out, "</abbr>");
		} else {
			mmd_print_const(out, ")</abbr>");
		}

		*first = !a->used;

		a->used = true;

		return 0;
	}

	return 1;
}


static int export_html_abbreviation_word(const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options, bool * first) {
	char * id = mmd_strndup(text, &len);

	// Insert '>''
	char * buffer = malloc(sizeof(char) * (len + 2));
	memcpy(&buffer[1], id, len + 1);
	buffer[0] = '>';

	abbr_def * a = read_ctx_get_abbr(r, buffer);
	free(id);
	free(buffer);

	return export_abbr_def(a, text, len, NULL, NULL, out, r, w, options, first);
}


static int export_endnote_def(unsigned char type, endnote_def * e, const char * text, size_t len, mmd_node * t, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	if (e) {
		const char * title = NULL;
		const char * class = NULL;
		char prefix = '\0';

		int idx = e->index;


		stack * s = NULL;

		// Set type specific variables
		switch (type) {
			case TOKEN_PAIR_BRACKET_CITATION:
				prefix = 'c';
				title = il8n[2][(int)r->language];
				class = "citation";
				s = w->used_cite_stack;
				break;

			case TOKEN_PAIR_BRACKET_FOOTNOTE:
				prefix = 'f';
				title = il8n[1][(int)r->language];
				class = "footnote";
				s = w->used_note_stack;
				break;

			case TOKEN_PAIR_BRACKET_GLOSSARY:
				prefix = 'g';
				title = il8n[3][(int)r->language];
				class = "glossary";
				s = w->used_glos_stack;
				break;
		}

		// Update index
		if (e->index == 0) {
			// Endnote has not been used previously

			// Add to appropriate used endnote stack
			stack_push(s, e);

			// Update index
			idx = (int) s->size;
			e->index = idx;
		}

		if (e->not_cited) {
			e->used = true;
			return 0;
		}

		// Export HTML based on endnote_def and type

		text_buffer_append_printf(out, "<a href=\"#%cn:%d\" ", prefix, idx);

		if (!e->used) {
			if (prefix == 'f' && (options & MMD_OPTION_RANDOM_NOTE_ID)) {
				text_buffer_append_printf(out, "id=\"%cnref:%d\" ", prefix, xorshift16(w->random_footnote_seed + idx));
			} else {
				text_buffer_append_printf(out, "id=\"%cnref:%d\" ", prefix, idx);
			}
		}

		text_buffer_append_printf(out, "title=\"%s\" class=\"%s\">", title, class);

		switch (type) {
			case TOKEN_PAIR_BRACKET_CITATION:

				// TODO: Print locator
				if (t && t->len) {
					mmd_print_const(out, "(");
					export_html_tokens(t, text, len, out, r, w, options);
					text_buffer_append_printf(out, ", %d)", idx);
				} else {
					text_buffer_append_printf(out, "(%d)", idx);
				}

				break;

			case TOKEN_PAIR_BRACKET_FOOTNOTE:
				text_buffer_append_printf(out, "<sup>%d</sup>", idx);
				break;

			case TOKEN_PAIR_BRACKET_GLOSSARY: {
				mmd_node * temp = (t) ? t->child : NULL;

				while (temp && temp->type != TOKEN_PAIR_PAREN) {
					temp = temp->next;
				}

				if (temp) {
					export_html_tokens(temp->child, text, len, out, r, w, options);
				} else {
					if (t) {
						export_html_tokens(t, text, len, out, r, w, options);
					} else {
						export_html_raw_text(e->key + 1, strlen(e->key) - 1, out);
					}
				}
			}
			break;
		}

		mmd_print_const(out, "</a>");

		e->used = true;

		return 0;
	}

	return 1;
}


static int export_html_glossary_word(const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	char * id = mmd_strndup(text, &len);

	// Insert '?'
	char * buffer = malloc(sizeof(char) * (len + 2));
	memcpy(&buffer[1], id, len + 1);
	buffer[0] = '?';

	endnote_def * e = read_ctx_get_glos(r, buffer);
	free(id);
	free(buffer);

	return export_endnote_def(TOKEN_PAIR_BRACKET_GLOSSARY, e, text, len, NULL, out, r, w, options);
}


static format_export fe = {
	&export_html_raw_text,
	&export_html_tokens,
	&export_html_link_def,
	&export_link_def_image,
	&export_abbr_def,
	&export_endnote_def
};


static void export_html_token(mmd_node ** t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	parse_rule rule = rules[(*t)->type];

	switch ((*t)->type) {
		case TOKEN_ATX_MARKER:
		case TOKEN_BLOCKQUOTE_MARKER:
		case TOKEN_CITATION_MARKER:
		case TOKEN_DEFLIST_COLON:
		case TOKEN_GLOSSARY_MARKER:
		case TOKEN_LIST_MARKER:
		case TOKEN_FOOTNOTE_MARKER:
		case OBJECT_REPLACEMENT_CHARACTER:
		case CODE_FENCE_LINE:
		case TOKEN_EOF:
		case TOKEN_MANUAL_LABEL:
		case TOKEN_TABLE_DIVIDER:
			// Ignore these completely
			break;

		// These are custom in order to speed up simple tokens that only have a single constant value.
		case TOKEN_APOSTROPHE:
		case TOKEN_DASH_M:
		case TOKEN_DASH_N:
		case TOKEN_ELLIPSIS:
		case TOKEN_NBSP:
			if (options & MMD_OPTION_COMPATIBILITY) {
				text_buffer_append_text(out, &text[(*t)->start], (*t)->len);
			} else {
				text_buffer_append_text(out, rule.prefix, rule.prefix_len);
			}

			break;

		case TOKEN_AMPERSAND:
		case TOKEN_AMPERSAND_LONG:
		case TOKEN_ANGLE_LEFT:
		case TOKEN_ANGLE_RIGHT:
		case TOKEN_TEXT_WHITESPACE:
			text_buffer_append_text(out, rule.prefix, rule.prefix_len);
			break;

		case TOKEN_DASH_N_RANGE:
			if (options & MMD_OPTION_COMPATIBILITY) {
				text_buffer_append_text(out, &text[(*t)->start], (*t)->len);
			} else {
				text_buffer_append_c(out, text[(*t)->start]);
				mmd_print_const(out, "&#8211;");
			}

			break;

		case TOKEN_ESCAPED_CHARACTER:
			export_html_raw_char(text[(*t)->start + 1], out);
			break;

		case TOKEN_LINEBREAK:
			if ((*t)->next) {
				mmd_print_const(out, "<br />\n");
				w->padding = 1;
			}

			break;

		case TOKEN_NL:
			if ((*t)->next) {
				mmd_print_const(out, "\n");
			}

			break;

		case TOKEN_TEXT_ABBREVIATION: {
			bool first = false;

			if (export_html_abbreviation_word(&text[(*t)->start], (*t)->len, out, r, w, options, &first)) {
				text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
			} else {
				// text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);

				// if (first) {
				// 	mmd_print_const(out, "</abbr>)");
				// } else {
				// 	mmd_print_const(out, "</abbr>");
				// }
			}
		}
		break;

		case TOKEN_TEXT_GLOSSARY:
			if (export_html_glossary_word(&text[(*t)->start], (*t)->len, out, r, w, options)) {
				text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
			}

			break;

		case TOKEN_PAIR_CM_SUB_DEL:
			mmd_print_const(out, "<del>");
			export_html_tokens((*t)->child, text, len, out, r, w, options);
			mmd_print_const(out, "</del>");

			if ((*t)->next && (*t)->next->type == TOKEN_PAIR_CM_SUB_ADD) {
				(*t) = (*t)->next;
				mmd_print_const(out, "<ins>");
				export_html_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "</ins>");
				(*t) = (*t)->next;
			}

			break;

		case TOKEN_PAIR_QUOTE_DOUBLE:
			if (options & MMD_OPTION_COMPATIBILITY) {
				mmd_print_const(out, "&quot;");
				export_html_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "&quot;");
			} else {
				text_buffer_append_text(out, double_quotes[(int)r->quotes_language].opener, double_quotes[(int)r->quotes_language].opener_len);
				export_html_tokens((*t)->child, text, len, out, r, w, options);
				text_buffer_append_text(out, double_quotes[(int)r->quotes_language].closer, double_quotes[(int)r->quotes_language].closer_len);
			}

			(*t) = (*t)->next;
			break;

		case TOKEN_PAIR_QUOTE_SINGLE:
			if (options & MMD_OPTION_COMPATIBILITY) {
				text_buffer_append_c(out, '\'');
				export_html_tokens((*t)->child, text, len, out, r, w, options);
				text_buffer_append_c(out, '\'');
			} else {
				text_buffer_append_text(out, single_quotes[(int)r->quotes_language].opener, single_quotes[(int)r->quotes_language].opener_len);
				export_html_tokens((*t)->child, text, len, out, r, w, options);
				text_buffer_append_text(out, single_quotes[(int)r->quotes_language].closer, single_quotes[(int)r->quotes_language].closer_len);
			}

			(*t) = (*t)->next;
			break;

		case TOKEN_PAIR_SUPERSCRIPT:
			mmd_print_const(out, "<sup>");
			export_html_tokens((*t)->child, text, len, out, r, w, options);
			mmd_print_const(out, "</sup>");

			if ((*t)->next && (*t)->next->type == TOKEN_SUPERSCRIPT) {
				(*t) = (*t)->next;
			}

			break;

		case TOKEN_SUPERSCRIPT:
			text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
			break;

		case TOKEN_PAIR_SUBSCRIPT:
			mmd_print_const(out, "<sub>");
			export_html_tokens((*t)->child, text, len, out, r, w, options);
			mmd_print_const(out, "</sub>");

			if ((*t)->next && (*t)->next->type == TOKEN_SUBSCRIPT) {
				(*t) = (*t)->next;
			}

			break;

		case TOKEN_SUBSCRIPT:
			text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
			break;

		case TOKEN_PAIR_BACKTICK:
			if (!(options & MMD_OPTION_COMPATIBILITY)) {
				if ((*t)->next && (*t)->next->next && ((*t)->next->next->type == TOKEN_PAIR_BRACE)) {
					mmd_node * n = (*t)->next->next;

					if (strncmp(&text[n->start], "{=", 2) == 0) {
						if (raw_filter_matches_format(&text[n->start], FORMAT_HTML)) {
							// Raw text that should be included for HTML format

							if ((*t)->child) {
								export_html_raw_text(&text[(*t)->child->start], (*t)->child->tail->start + (*t)->child->tail->len - (*t)->child->start, out);
							}

						}

						(*t) = n->next;
						break;
					}
				}
			}

			text_buffer_append_text(out, rule.prefix, rule.prefix_len);

			if ((*t)->child) {
				export_html_raw_text(&text[(*t)->child->start], (*t)->child->tail->start + (*t)->child->tail->len - (*t)->child->start, out);
			}

			text_buffer_trim_trailing_whitespace(out);

			text_buffer_append_text(out, rule.suffix, rule.suffix_len);

			(*t) = (*t)->next;

			break;

		case TOKEN_PAIR_BRACKET_EMPTY:
		case TOKEN_PAIR_BRACKET_NOT_CITED:
			if (export_token_pair(text, len, t, out, r, w, &fe, options)) {
				text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
			}

			break;

		case TOKEN_PAIR_BRACKET:
		case TOKEN_PAIR_BRACKET_IMAGE:
		case TOKEN_PAIR_BRACKET_ABBREVIATION:
		case TOKEN_PAIR_BRACKET_CITATION:
		case TOKEN_PAIR_BRACKET_FOOTNOTE:
		case TOKEN_PAIR_BRACKET_GLOSSARY:
		case TOKEN_PAIR_BRACKET_VARIABLE:
			if (w->in_endnote && w->skip_endnote_label) {
				// Skip label and colon
				(*t) = (*t)->next;
				w->skip_endnote_label = false;
			} else {
				if (export_token_pair(text, len, t, out, r, w, &fe, options)) {
					// Plain [text] since parsing failed

					switch ((*t)->type) {
						case TOKEN_PAIR_BRACKET_ABBREVIATION:
						case TOKEN_PAIR_BRACKET_CITATION:
						case TOKEN_PAIR_BRACKET_FOOTNOTE:
						case TOKEN_PAIR_BRACKET_GLOSSARY:
						case TOKEN_PAIR_BRACKET_VARIABLE:
							// We'll need to print this character
							(*t)->child->type = TOKEN_TEXT;
							break;

						default:
							break;
					}

					text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
					export_html_tokens((*t)->child, text, len, out, r, w, options);
				}
			}

			break;

		case TOKEN_PAIR_ANGLE:
			if ((*t)->child && scan_url(&text[(*t)->child->start]) == (*t)->next->start - (*t)->start - (*t)->len) {
				// This is an "automatic" link, e.g. <http://...>
				mmd_print_const(out, "<a href=\"");
				export_html_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "\">");
				export_html_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "</a>");
				(*t) = (*t)->next;
			} else if ((*t)->child && scan_email(&text[(*t)->child->start]) == (*t)->next->start - (*t)->start - (*t)->len) {
				// This is an "automatic" mailto, e.g. <mailto:...>
				mmd_print_const(out, "<a href=\"");

				if (strncmp("mailto:", &text[(*t)->child->start], 7)) {
					mmd_print_const(out, "mailto:");
				}

				export_html_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "\">");
				export_html_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "</a>");
				(*t) = (*t)->next;
			} else if (scan_html(&text[(*t)->start])) {
#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
				export_text_except_cr(&text[(*t)->start], (int)((*t)->next->start + (*t)->next->len - (*t)->start), out);
#else
				text_buffer_append_text(out, &text[(*t)->start], (int)((*t)->next->start + (*t)->next->len - (*t)->start));
#endif
				(*t) = (*t)->next;
			} else {
				// This is plain text that happens to be wrapped in <...>
				// <html> is handled elsewhere
				mmd_print_const(out, "&lt;");
				export_html_tokens((*t)->child, text, len, out, r, w, options);
			}

			break;

		case TOKEN_TABLE_CELL:
			pad(out, 1, w);

			if (w->in_table_header) {
				mmd_print_const(out, "\t<th");
			} else {
				mmd_print_const(out, "\t<td");
			}

			// Handle column alignment
			switch (w->table_alignment[w->table_col_count++]) {
				case 'r':
				case 'R':
					mmd_print_const(out, " style=\"text-align:right;\"");
					break;

				case 'c':
				case 'C':
					mmd_print_const(out, " style=\"text-align:center;\"");
					break;

				case 'l':
				case 'L':
					mmd_print_const(out, " style=\"text-align:left;\"");
					break;
			}

			// Handle colspan
			if ((*t)->next && (*t)->next->type == TOKEN_TABLE_DIVIDER) {
				if ((*t)->next->len > 1) {
					text_buffer_append_printf(out, " colspan=\"%d\"", (*t)->next->len);
				}
			}

			mmd_print_const(out, ">");
			w->padding = 0;

			// Print cell contents
			export_html_tokens((*t)->child, text, (*t)->len, out, r, w, options);

			if (w->in_table_header) {
				mmd_print_const(out, "</th>");
			} else {
				mmd_print_const(out, "</td>");
			}

			break;

		default:
			if (rule.prefix) {
				text_buffer_append_text(out, rule.prefix, rule.prefix_len);
				w->padding = rule.pre_descent_padding;
			}

			if (rule.descent & DESCEND_CHILD) {
				if ((*t)->child) {
					export_html_tokens((*t)->child, text, len, out, r, w, options);
				}
			}

			if (rule.descent & DESCEND_CONTENT) {
			}

			if (rule.descent & DESCEND_RAW) {
				if ((*t)->child) {
					export_html_raw_text(&text[(*t)->child->start], (*t)->child->tail->start + (*t)->child->tail->len - (*t)->child->start, out);
				}
			}

			if (rule.descent == VERBATIM) {
				// Default to printing token
				text_buffer_append_text(out, &text[(*t)->start], (*t)->len);
			}

			if (rule.suffix) {
				text_buffer_append_text(out, rule.suffix, rule.suffix_len);
			}

			F(i, rule.skip) {
				(*t) = (*t)->next;
			}

			w->padding = 0;
			break;
	}
}


void export_html_tokens(mmd_node * t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	while (t) {
		export_html_token(&t, text, len, out, r, w, options);

		t = t->next;
	}
}


static void export_html_block(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	parse_rule rule = rules[b->type];

	switch (b->type) {
		case BLOCK_DEF_ABBREVIATION:
		case BLOCK_DEF_CITATION:
		case BLOCK_DEF_FOOTNOTE:
		case BLOCK_DEF_GLOSSARY:
		case BLOCK_DEF_LINK:
		case BLOCK_EMPTY:
		case BLOCK_META:
			// Ignore these completely
			break;

		case BLOCK_CODE_FENCED: {
			pad(out, 2, w);
			char * lang = copy_fence_language_specificer(&text[b->start]);

			if (lang) {
				if (strncmp("{=", lang, 2) == 0) {
					// This is raw source
					if (raw_filter_matches_format(lang, FORMAT_HTML)) {
						w->padding = 0;

						if (w->in_recursive) {
							export_html_lines_raw_content((mmd_line_node *)b->child, &text[b->start], out);
						} else {
							export_html_lines_raw((mmd_line_node *)b->child, &text[b->start], out);
						}

						w->padding = 1;
					}

					free(lang);
					break;
				}

				text_buffer_append_printf(out, "<pre><code class=\"%s\">", lang);
				free(lang);
			} else {
				mmd_print_const(out, "<pre><code>");
			}

			w->padding = 0;

			if (w->in_recursive) {
				export_html_lines_raw_content((mmd_line_node *)b->child, &text[b->start], out);
			} else {
				export_html_lines_raw((mmd_line_node *)b->child, &text[b->start], out);
			}

			mmd_print_const(out, "</code></pre>");
			w->padding = 0;
		}

		break;

		case BLOCK_DEFLIST:
			pad(out, 2, w);

			// Group consecutive definition lists together
			if (!w->in_definition_list) {
				mmd_print_const(out, "<dl>\n");
			}

			w->padding = 2;

			export_html_blocks(b->child, &text[b->start], out, r, w, options);
			pad(out, 1, w);

			if (b->next && b->next->type == BLOCK_DEFLIST) {
				w->in_definition_list = true;
			} else {
				w->in_definition_list = false;
				mmd_print_const(out, "</dl>\n");
			}

			w->padding = 1;
			break;

		case BLOCK_DEFINITION:
			pad(out, 2, w);
			mmd_print_const(out, "<dd>");
			w->padding = 2;

			if (b->child->next) {
				export_html_blocks(b->child, &text[b->start], out, r, w, options);
			} else {
				export_html_tokens(b->child->content, &text[b->start], b->len, out, r, w, options);
			}

			mmd_print_const(out, "</dd>\n");
			w->padding = 2;
			break;

		case BLOCK_H1:
		case BLOCK_H2:
		case BLOCK_H3:
		case BLOCK_H4:
		case BLOCK_H5:
		case BLOCK_H6: {
			pad(out, 2, w);

			int level = b->type - BLOCK_H1 + read_ctx_get_header_level(r, FORMAT_HTML);

			if (options & MMD_OPTION_COMPATIBILITY) {
				if ((level >= 0) && (level < 6)) {
					text_buffer_append_text(out, headers_compat[level].opener, headers_compat[level].opener_len);
				}
			} else {
				if ((level >= 0) && (level < 6)) {
					text_buffer_append_text(out, headers[level].opener, headers[level].opener_len);
				}

				header * h = stack_peek_index(r->header_stack, w->header_count++);

				if (h) {
					export_html_raw_text(h->key, strlen(h->key), out);
				} else {
					char * id = html_id_from_text(&text[b->start], b->len, true);
					export_html_raw_text(id, strlen(id), out);
					free(id);
				}

				mmd_print_const(out, "\">");
			}

			export_html_tokens(b->content, &text[b->start], b->len, out, r, w, options);

			text_buffer_trim_trailing_whitespace(out);

			if ((level >= 0) && (level < 6)) {
				text_buffer_append_text(out, headers[level].closer, headers[level].closer_len);
			}

			w->padding = 0;
		}
		break;

		case BLOCK_SETEXT_1:
		case BLOCK_SETEXT_2: {
			pad(out, 2, w);

			int level = b->type - BLOCK_SETEXT_1 + read_ctx_get_header_level(r, FORMAT_HTML);

			if (options & MMD_OPTION_COMPATIBILITY) {
				if ((level >= 0) && (level < 6)) {
					text_buffer_append_text(out, headers_compat[level].opener, headers_compat[level].opener_len);
				}
			} else {
				if ((level >= 0) && (level < 6)) {
					text_buffer_append_text(out, headers[level].opener, headers[level].opener_len);
				}

				header * h = stack_peek_index(r->header_stack, w->header_count++);

				if (h) {
					export_html_raw_text(h->key, strlen(h->key), out);
				} else {
					char * id = html_id_from_text(&text[b->start], b->len - b->child->tail->len, true);
					export_html_raw_text(id, strlen(id), out);
					free(id);
				}

				mmd_print_const(out, "\">");
			}

			export_html_tokens(b->content, &text[b->start], b->len, out, r, w, options);

			text_buffer_trim_trailing_whitespace(out);

			if ((level >= 0) && (level < 6)) {
				text_buffer_append_text(out, headers[level].closer, headers[level].closer_len);
			}

			w->padding = 0;
		}
		break;

		case BLOCK_LIST_ITEM_TIGHT:
			// Custom because we need to handle the first child differently
			pad(out, 1, w);
			mmd_print_const(out, "<li>");
			w->padding = 2;

			if (MMD_NODE_IS_BLOCK(b->child)) {
				if (b->child->type == BLOCK_PARA) {
					export_html_tokens(b->child->content, &text[b->start], b->len, out, r, w, options);
				} else {
					export_html_block(b->child, &text[b->start], out, r, w, options);
				}
			} else {
				export_html_tokens(b->child->content, &text[b->start], b->len, out, r, w, options);
			}

			w->padding = 0;
			export_html_blocks(b->child->next, &text[b->start], out, r, w, options);
			mmd_print_const(out, "</li>");
			w->padding = 0;
			break;

		case BLOCK_PARA:
			pad(out, 2, w);
			mmd_print_const(out, "<p>");
			w->padding = 0;
			export_html_tokens(b->content, &text[b->start], b->len, out, r, w, options);

			if (w->in_endnote) {
				if (w->endnote_return) {
					w->endnote_return_para_counter--;

					if (w->endnote_return_para_counter == 0) {
						text_buffer_append_text(out, w->endnote_return, (int)strlen(w->endnote_return));
						free(w->endnote_return);
						w->endnote_return = NULL;
					}
				}
			}

			mmd_print_const(out, "</p>");
			w->padding = 0;
			break;

		case BLOCK_TABLE:
			pad(out, 2, w);
			mmd_print_const(out, "<table");

			// Is there a caption?
			{
				char * id = table_label(b, text);

				if (id) {
					text_buffer_append_printf(out, " id=\"%s\">\n<caption style=\"caption-side: bottom;\">", id);
					free(id);
					export_html_tokens(b->next->content->child, &text[b->next->start], b->next->len, out, r, w, options);
					mmd_print_const(out, "</caption>\n");
				} else {
					mmd_print_const(out, ">\n");
				}
			}

			// Handle column setup and alignment
			write_ctx_get_table_alignments(w, b, &text[b->start]);
			mmd_print_const(out, "<colgroup>\n");
			F(i, w->table_col_count) {
				switch (w->table_alignment[i]) {
					case 'l':
						mmd_print_const(out, "<col style=\"text-align:left;\"/>\n");
						break;

					case 'N':
					case 'L':
						mmd_print_const(out, "<col style=\"text-align:left;\" class=\"extended\"/>\n");
						break;

					case 'r':
						mmd_print_const(out, "<col style=\"text-align:right;\"/>\n");
						break;

					case 'R':
						mmd_print_const(out, "<col style=\"text-align:right;\" class=\"extended\"/>\n");
						break;

					case 'c':
						mmd_print_const(out, "<col style=\"text-align:center;\"/>\n");
						break;

					case 'C':
						mmd_print_const(out, "<col style=\"text-align:center;\" class=\"extended\"/>\n");
						break;

					default:
						mmd_print_const(out, "<col />\n");
						break;
				}
			}

			mmd_print_const(out, "</colgroup>\n\n");
			w->padding = 2;

			// Export table rows
			export_html_blocks(b->child, &text[b->start], out, r, w, options);
			pad(out, 1, w);
			mmd_print_const(out, "</table>");
			w->padding = 0;

			if (table_has_caption(b)) {
				// Don't export caption again
				w->skip_blocks = 1;
			}

			break;

		case BLOCK_TABLE_SEPARATOR:
			if (!w->in_table_header) {
				pad(out, 1, w);
				mmd_print_const(out, "<tr>\n");
				w->padding = 1;
				w->table_col_count = 0;
				export_html_tokens(b->content, &text[b->start], b->len, out, r, w, options);
				pad(out, 1, w);
				mmd_print_const(out, "</tr>\n");
				w->padding = 0;

			}

			break;

		case BLOCK_TOC:
			pad(out, 2, w);
			mmd_print_const(out, "<div class=\"TOC\">\n");

			export_html_toc(&text[b->start], out, r, w, options);

			mmd_print_const(out, "</div>");
			w->padding = 0;
			break;

		default:
			pad(out, rule.padding, w);

			if (rule.prefix) {
				text_buffer_append_text(out, rule.prefix, rule.prefix_len);
				w->padding = rule.pre_descent_padding;
			}

			switch (b->type) {
				case BLOCK_TABLE_ROW:
					w->table_col_count = 0;
					break;

				case BLOCK_FIGURE:
					w->in_figure = true;
					break;

				case BLOCK_TABLE_HEADER:
					w->in_table_header = true;
					break;

				case BLOCK_BLOCKQUOTE:
				case BLOCK_LIST_BULLETED:
				case BLOCK_LIST_BULLETED_LOOSE:
				case BLOCK_LIST_ENUMERATED:
				case BLOCK_LIST_ENUMERATED_LOOSE:
					w->in_recursive = true;
					break;
			}

			if (rule.descent & DESCEND_CHILD) {
				export_html_blocks(b->child, &text[b->start], out, r, w, options);
			}

			if (rule.descent & DESCEND_CONTENT) {
				export_html_tokens(b->content, &text[b->start], b->len, out, r, w, options);
			}

			if (rule.descent & DESCEND_LINES) {
				if (w->in_recursive) {
					export_html_lines_content(b->child, &text[b->start], out);
				} else {
					export_html_lines(b->child, &text[b->start], out);
				}
			}

			if (rule.descent & DESCEND_LINES_RAW) {
				export_html_lines_raw_content((mmd_line_node *)b->child, &text[b->start], out);
			}

			if (rule.descent & DESCEND_RAW) {

			}

			switch (b->type) {
				case BLOCK_FIGURE:
					w->in_figure = false;
					break;

				case BLOCK_TABLE_HEADER:
					w->in_table_header = false;
					break;

				case BLOCK_BLOCKQUOTE:
				case BLOCK_LIST_BULLETED:
				case BLOCK_LIST_BULLETED_LOOSE:
				case BLOCK_LIST_ENUMERATED:
				case BLOCK_LIST_ENUMERATED_LOOSE:
					w->in_recursive = false;
					break;
			}

			pad(out, rule.pad_post_descent, w);

			if (rule.suffix) {
				text_buffer_append_text(out, rule.suffix, rule.suffix_len);
			}

			w->padding = rule.post_suffix_padding;
			break;
	}
}


static void export_html_blocks(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	while (b) {
		if (w->skip_blocks) {
			w->skip_blocks--;
		} else {
			export_html_block(b, text, out, r, w, options);
		}

		b = b->next;
	}
}


static void export_html_header(text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	meta * m;

	mmd_print_const(out, "<!DOCTYPE html>\n<html xmlns=\"http://www.w3.org/1999/xhtml\"");
	m = read_ctx_get_meta(r, "language");

	if (m) {
		text_buffer_append_printf(out, " lang=\"%s\"", m->value);
	} else {
		mmd_print_const(out, " lang=\"en\"");
	}

	mmd_print_const(out, ">\n<head>\n\t<meta charset=\"utf-8\"/>\n");

	// Iterate over metadata keys
	for (m = r->meta_hash; m != NULL; m = m->hh.next) {
		switch (m->key[0]) {
			case 'b':
				if (strcmp(m->key, "baseheaderlevel") == 0) {
					continue;
				} else if (strcmp(m->key, "bibliostyle") == 0) {
					continue;
				} else if (strcmp(m->key, "bibtex") == 0) {
					continue;
				}

				break;

			case 'c':
				if (strcmp(m->key, "css") == 0) {
					if (options & MMD_OPTION_EMBED_ASSETS) {
						asset * a = read_ctx_store_asset(r, m->value, m->value_len, options, g_search_path);

						if (a && a->len) {
							mmd_print_const(out, "\t<style>\n");
							text_buffer_append_text(out, a->data, a->len);
							mmd_print_const(out, "\t</style>\n");
							continue;
						}
					}

					mmd_print_const(out, "\t<link type=\"text/css\" rel=\"stylesheet\" href=\"");

					if (options & MMD_OPTION_STORE_ASSETS) {
						asset * a = read_ctx_store_asset(r, m->value, m->value_len, options, g_search_path);

						if (a) {
							mmd_print_const(out, "assets/");
							text_buffer_append_text(out, a->uuid, 36);
						} else {
							url_encode_text(m->value, m->value_len, out);
						}
					} else {
						url_encode_text(m->value, m->value_len, out);
					}

					mmd_print_const(out, "\"/>\n");

					continue;
				}

				break;

			case 'h':
				if (strcmp(m->key, "htmlfooter") == 0) {
					continue;
				} else if (strcmp(m->key, "htmlheader") == 0) {
					text_buffer_append_text(out, m->value, (int)m->value_len);
					text_buffer_append_c(out, '\n');
					continue;
				} else if (strcmp(m->key, "htmlheaderlevel") == 0) {
					continue;
				}

				break;

			case 'l':
				if (strcmp(m->key, "language") == 0) {
					continue;
				} else if (strcmp(m->key, "latexbegin") == 0) {
					continue;
				} else if (strcmp(m->key, "latexconfig") == 0) {
					continue;
				} else if (strcmp(m->key, "latexfooter") == 0) {
					continue;
				} else if (strcmp(m->key, "latexheader") == 0) {
					continue;
				} else if (strcmp(m->key, "latexheaderlevel") == 0) {
					continue;
				} else if (strcmp(m->key, "latexinput") == 0) {
					continue;
				} else if (strcmp(m->key, "latexleader") == 0) {
					continue;
				} else if (strcmp(m->key, "latexmode") == 0) {
					continue;
				}

				break;

			case 'm':
				if (strcmp(m->key, "mmdfooter") == 0) {
					continue;
				} else if (strcmp(m->key, "mmdheader") == 0) {
					continue;
				}

				break;

			case 'o':
				if (strcmp(m->key, "odfheader") == 0) {
					continue;
				}

				break;

			case 'q':
				if (strcmp(m->key, "quoteslanguage") == 0) {
					continue;
				}

				break;

			case 't':
				if (strcmp(m->key, "title") == 0) {
					mmd_print_const(out, "\t<title>");
					export_html_raw_text(m->value, m->value_len, out);
					mmd_print_const(out, "</title>\n");
					continue;
				} else if (strcmp(m->key, "transcludebase") == 0) {
					continue;
				}

				break;

			case 'x':
				if (strcmp(m->key, "xhtmlheader") == 0) {
					text_buffer_append_text(out, m->value, (int)m->value_len);
					text_buffer_append_c(out, '\n');
					continue;
				} else if (strcmp(m->key, "xhtmlheaderlevel") == 0) {
					continue;
				}
		}

		// Any other metadata comes here
		mmd_print_const(out, "\t<meta name=\"");
		text_buffer_append_printf(out, "%s", m->key);
		mmd_print_const(out, "\" content=\"");
		text_buffer_append_text(out, m->value, (int)m->value_len);
		mmd_print_const(out, "\"/>\n");
	}

	mmd_print_const(out, "</head>\n<body>\n");
	w->padding = 2;
}


static void export_html_endnote_section(text_buffer * out, stack * s, const char * class, const char * reverseclass, char prefix, read_ctx * r, write_ctx * w, uint32_t options) {
	if (s->size) {
		pad(out, 2, w);
		text_buffer_append_printf(out, "<div class=\"%s\">\n<hr />\n<ol>\n", class);
		w->padding = 1;

		F(i, (int)s->size) {
			endnote_def * e = stack_peek_index(s, i);

			pad(out, 2, w);
			text_buffer_append_printf(out, "<li id=\"%cn:%d\">\n", prefix, e->index);
			w->padding = 2;

			if (e->title_node) {
				// Only used in glossary entries
				export_html_tokens(e->title_node, e->text, e->len, out, r, w, options);
				mmd_print_const(out, ": ");
				w->padding = 2;
			}

			w->in_endnote = 1;

			if (!e->is_inline) {
				w->skip_endnote_label = true;
			}

			if (e->content_node && MMD_NODE_IS_BLOCK(e->content_node)) {
				w->endnote_return = calloc(1, sizeof(char) * 512);

				if (prefix == 'f' && (options & MMD_OPTION_RANDOM_NOTE_ID)) {
					snprintf(w->endnote_return, 512 - 1, " <a href=\"#%cnref:%d\" title=\"%s\" class=\"%s\">&#160;&#8617;&#xfe0e;</a>", prefix, xorshift16(w->random_footnote_seed + e->index), il8n[0][(int)r->language], reverseclass);
				} else {
					snprintf(w->endnote_return, 512 - 1, " <a href=\"#%cnref:%d\" title=\"%s\" class=\"%s\">&#160;&#8617;&#xfe0e;</a>", prefix, e->index, il8n[0][(int)r->language], reverseclass);
				}

				w->endnote_return_para_counter = 0;

				mmd_node * walker = e->content_node;

				while (walker) {
					if (walker->type == BLOCK_PARA) {
						w->endnote_return_para_counter++;
					}

					walker = walker->next;
				}

				export_html_blocks(e->content_node, e->text, out, r, w, options);
				free(w->endnote_return);
			} else {
				mmd_print_const(out, "<p>");
				export_html_tokens(e->content_node, e->text, e->len, out, r, w, options);

				if (prefix == 'f' && (options & MMD_OPTION_RANDOM_NOTE_ID)) {
					text_buffer_append_printf(out, " <a href=\"#%cnref:%d\" title=\"%s\" class=\"%s\">&#160;&#8617;&#xfe0e;</a>", prefix, xorshift16(w->random_footnote_seed + e->index), il8n[0][(int)r->language], reverseclass);
				} else {
					text_buffer_append_printf(out, " <a href=\"#%cnref:%d\" title=\"%s\" class=\"%s\">&#160;&#8617;&#xfe0e;</a>", prefix, e->index, il8n[0][(int)r->language], reverseclass);
				}

				mmd_print_const(out, "</p>");
				w->padding = 0;
			}

			w->in_endnote = 0;
			w->skip_endnote_label = false;

			pad(out, 1, w);
			mmd_print_const(out, "</li>\n");
			w->padding = 1;
		}

		pad(out, 2, w);
		mmd_print_const(out, "</ol>\n</div>\n");
		w->padding = 1;
	}
}


static void export_html_endnotes(text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	export_html_endnote_section(out, w->used_note_stack, "footnotes", "reversefootnote", 'f', r, w, options);
	export_html_endnote_section(out, w->used_glos_stack, "glossary", "reverseglossary", 'g', r, w, options);
	export_html_endnote_section(out, w->used_cite_stack, "citations", "reversecitation", 'c', r, w, options);
}


static void export_html_footer(text_buffer * out, read_ctx * r, write_ctx * w) {
	pad(out, 1, w);

	// Iterate over metadata keys
	for (meta * m = r->meta_hash; m != NULL; m = m->hh.next) {
		if (strcmp(m->key, "htmlfooter") == 0) {
			text_buffer_append_text(out, m->value, (int)m->value_len);
			text_buffer_append_c(out, '\n');
		}
	}

	mmd_print_const(out, "</body>\n</html>\n");
	w->padding = 1;
}


void export_html(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, uint32_t options, const char * source_path) {
	// Store thread local copy of search path
	g_search_path = mmd_dirname(source_path);

	precalculate_rules(rules, sizeof(rules) / sizeof(rules[0]));
	precalculate_quotes(single_quotes, sizeof(single_quotes) / sizeof((single_quotes[0])));
	precalculate_quotes(double_quotes, sizeof(double_quotes) / sizeof(double_quotes[0]));
	precalculate_quotes(headers, sizeof(headers) / sizeof(headers[0]));
	precalculate_quotes(headers_compat, sizeof(headers_compat) / sizeof(headers_compat[0]));

	write_ctx * w = write_ctx_new();

	if (r->write_complete || (r->has_meta && !r->write_snippet)) {
		export_html_header(out, r, w, options);
	}

	export_html_blocks(b, text, out, r, w, options);

	export_html_endnotes(out, r, w, options);

	if (r->write_complete || (r->has_meta && !r->write_snippet)) {
		export_html_footer(out, r, w);
	}

	pad(out, 1, w);
	write_ctx_free(w);

	free(g_search_path);
	g_search_path = NULL;
}
