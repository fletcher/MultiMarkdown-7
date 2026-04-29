/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file latex.c

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
#include "latex.h"


#ifdef TEST
	#include "CuTest.h"
#endif

#define F(i,n) for(int i= 0;i<n;i++)

static void export_latex_tokens(mmd_node * t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options);
static void export_latex_blocks(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	__declspec(thread) int g_format = 0;
	__declspec(thread) int g_in_frame = 0;
#else
	__thread int g_format = 0;
	__thread int g_in_frame = 0;
#endif


static parse_rule rules[256] = {
	[BLOCK_BLOCKQUOTE]				= { 2, "\\begin{quote}", 1, DESCEND_CHILD, 1, "\\end{quote}", 0, 0, 0, 0 },
	[BLOCK_CODE_INDENTED]			= { 2, "\\begin{verbatim}\n", 0, DESCEND_LINES_VERBATIM, 0, "\\end{verbatim}", 0, 0, 0, 0 },
	[BLOCK_FIGURE]					= { 2, NULL, 0, DESCEND_CONTENT, 0, NULL, 0, 0, 0, 0 },
	[BLOCK_HR]						= { 2, "\\begin{center}\\rule{3in}{0.4pt}\\end{center}", 0, 0, 0, NULL, 0, 0, 0, 0 },
	[BLOCK_HTML]					= { 0, NULL, 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[BLOCK_LIST_BULLETED]			= { 2, "\\begin{itemize}", 0, DESCEND_CHILD, 1, "\\end{itemize}", 0, 0, 0, 0 },
	[BLOCK_LIST_BULLETED_LOOSE]		= { 2, "\\begin{itemize}", 0, DESCEND_CHILD, 1, "\\end{itemize}", 0, 0, 0, 0 },
	[BLOCK_LIST_ENUMERATED]			= { 2, "\\begin{enumerate}", 0, DESCEND_CHILD, 1, "\\end{enumerate}", 0, 0, 0, 0 },
	[BLOCK_LIST_ENUMERATED_LOOSE]	= { 2, "\\begin{enumerate}", 0, DESCEND_CHILD, 1, "\\end{enumerate}", 0, 0, 0, 0 },
	[BLOCK_LIST_ITEM]				= { 1, "\\item ", 2, DESCEND_CHILD, 0, "", 0, 0, 0, 0 },
	[BLOCK_PARA]					= { 2, "", 0, DESCEND_CONTENT, 0, "", 0, 0, 0, 0 },
	[BLOCK_TERM]					= { 2, "\\item[", 0, DESCEND_CONTENT, 0, "]\n", 2, 0, 0, 0 },

	[BLOCK_TABLE_HEADER]			= { 1, "\\begin{tabulary}{\\textwidth}", 0, DESCEND_CHILD, 1, "\\midrule", 0, 0, 0, 0 },
	[BLOCK_TABLE_SECTION]			= { 1, "", 0, DESCEND_CHILD, 1, "\\bottomrule", 0, 0, 0, 0 },
	[BLOCK_TABLE_ROW]				= { 1, "", 1, DESCEND_CONTENT, 0, "\\\\", 0, 0, 0, 0 },
	[BLOCK_TABLE_SEPARATOR]			= { 1, "", 1, DESCEND_CONTENT, 0, "\\\\", 0, 0, 0, 0 },
	[LINE_TABLE_SEPARATOR]			= { 1, "", 1, DESCEND_CONTENT, 0, "\\\\", 0, 0, 0, 0 },

	[TOKEN_PAIR_CM_SUB_ADD]			= { 0, "\\ensuremath{\\sim}>", 0, DESCEND_CHILD, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_PAIR_CM_ADD]				= { 0, "\\underline{", 0, DESCEND_CHILD, 0, "}", 0, 1, 0, 0 },
	[TOKEN_PAIR_CM_DEL]				= { 0, "\\sout{", 0, DESCEND_CHILD, 0, "}", 0, 1, 0, 0 },
	[TOKEN_PAIR_CM_COM]				= { 0, "\\cmnote{", 0, DESCEND_CHILD, 0, "}", 0, 1, 0, 0 },
	[TOKEN_PAIR_CM_HI]				= { 0, "\\hl{", 0, DESCEND_CHILD, 0, "}", 0, 1, 0, 0 },

	[TOKEN_PAIR_BACKTICK]			= { 0, "\\texttt{", 0, DESCEND_RAW, 0, "}", 0, 1, 0, 0 },
	[TOKEN_PAIR_EMPH]				= { 0, "\\emph{", 0, DESCEND_CHILD, 0, "}", 0, 1, 0, 0 },
	[TOKEN_PAIR_MATH_PAREN]			= { 0, "\\(", 0, DESCEND_VERBATIM, 0, "\\)", 0, 1, 0, 0 },
	[TOKEN_PAIR_MATH_BRACKET]		= { 0, "\\[", 0, DESCEND_VERBATIM, 0, "\\]", 0, 1, 0, 0 },
	[TOKEN_PAIR_MATH_DOLLAR_SINGLE]	= { 0, "$", 0, DESCEND_VERBATIM, 0, "$", 0, 1, 0, 0 },
	[TOKEN_PAIR_MATH_DOLLAR_DOUBLE]	= { 0, "$$", 0, DESCEND_VERBATIM, 0, "$$", 0, 1, 0, 0 },
	[TOKEN_PAIR_STRONG]				= { 0, "\\textbf{", 0, DESCEND_CHILD, 0, "}", 0, 1, 0, 0 },
	[TOKEN_PAIR_STAR]				= { 0, "*", 0, DESCEND_CHILD, 0, "*", 0, 1, 0, 0 },
	[TOKEN_PAIR_STAR_USED]			= { 0, NULL, 0, DESCEND_CHILD, 0, NULL, 0, 1, 0, 0 },
	[TOKEN_PAIR_UL]					= { 0, "_", 0, DESCEND_CHILD, 0, "_", 0, 1, 0, 0 },
	[TOKEN_PAIR_UL_USED]			= { 0, NULL, 0, DESCEND_CHILD, 0, NULL, 0, 1, 0, 0 },
	[TOKEN_PAIR_PAREN]				= { 0, "(", 0, DESCEND_CHILD, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_PAIR_BRACE]				= { 0, "\\{", 0, DESCEND_CHILD, 0, NULL, 0, 0, 0, 0 },

	[TOKEN_ANGLE_LEFT]				= { 0, "<", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_ANGLE_RIGHT]				= { 0, ">", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_APOSTROPHE]				= { 0, "'", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_BRACE_LEFT]				= { 0, "\\{", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_BRACE_RIGHT]				= { 0, "\\}", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_DASH_M]					= { 0, "---", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_DASH_N]					= { 0, "--", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_ELLIPSIS]				= { 0, "{\\ldots}", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_AMPERSAND]				= { 0, "\\&", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_AMPERSAND_LONG]			= { 0, "\\&", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_NBSP]					= { 0, "~", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_QUOTE_DOUBLE]			= { 0, "''", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_TEXT_WHITESPACE]			= { 0, " ", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_UL]						= { 0, "\\_", 0, NONE, 0, NULL, 0, 0, 0, 0 },

	[TOKEN_CM_COM_OPEN]				= { 0, "\\{>>", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_CM_COM_CLOSE]			= { 0, "\\ensuremath{\\sim}\\ensuremath{\\sim}\\}", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_CM_SUB_DIV]				= { 0, "\\ensuremath{\\sim}>", 0, NONE, 0, NULL, 0, 0, 0, 0 },
	[TOKEN_CM_SUB_CLOSE]			= { 0, "\\ensuremath{\\sim}\\ensuremath{\\sim}\\}", 0, NONE, 0, NULL, 0, 0, 0, 0 },
};


static smart_quote single_quotes[16] = {
	[QUOTES_ENGLISH]			= { "`", "'", 0, 0 },
	[QUOTES_DUTCH]				= { "`", "'", 0, 0 },
	[QUOTES_SWEDISH]			= { "'", "'", 0, 0 },
	[QUOTES_FRENCH]				= { "'", "'", 0, 0 },
	[QUOTES_GERMAN]				= { "‚", "`", 0, 0 },
	[QUOTES_GERMAN_GUILLEMETS]	= { "›", "‹", 0, 0 },
	[QUOTES_SPANISH]			= { "`", "'", 0, 0 },
};


static smart_quote double_quotes[16] = {
	[QUOTES_ENGLISH]			= { "``", "''", 0, 0 },
	[QUOTES_DUTCH]				= { "„", "''", 0, 0 },
	[QUOTES_SWEDISH]			= { "''", "''", 0, 0 },
	[QUOTES_FRENCH]				= { "«", "»", 0, 0 },
	[QUOTES_GERMAN]				= { "„", "``", 0, 0 },
	[QUOTES_GERMAN_GUILLEMETS]	= { "»", "«", 0, 0 },
	[QUOTES_SPANISH]			= { "«", "»", 0, 0 },
};


static smart_quote headers[][6] = {
	[FORMAT_LATEX]				= {
		{ "\\part{", "}", 0, 0 },
		{ "\\chapter{", "}", 0, 0 },
		{ "\\section{", "}", 0, 0 },
		{ "\\subsection{", "}", 0, 0 },
		{ "\\subsubsection{", "}", 0, 0 },
		{ "\\paragraph{", "}", 0, 0 },
	},
	[FORMAT_BEAMER]				= {
		{ "\\part{", "}", 0, 0 },
		{ "\\section{", "}", 0, 0 },
		{ "\\subsection{", "}", 0, 0 },
		{ "\\begin{frame}[fragile]\n\\frametitle{", "}", 0, 0 },
		{ "\\emph{", "}", 0, 0 },
		{ "\\emph{", "}", 0, 0 },
	},
	[FORMAT_LTX_TALK]			= {
		{ "\\part{", "}", 0, 0 },
		{ "\\section{", "}", 0, 0 },
		{ "\\subsection{", "}", 0, 0 },
		// { "\\begin{frame}\n\\frametitle{", "}", 0, 0 },
		{ "\\begin{frame}{", "}", 0, 0 },
		{ "\\emph{", "}", 0, 0 },
		{ "\\emph{", "}", 0, 0 },
	},
};


/// Embed a latex \input{} file directly in the output
static void embed_input_file(text_buffer * out, const char * fname) {
	FILE * fp;
	char buffer[1024];
	char command[1024];

	snprintf(command, sizeof(command), "kpsewhich %s", fname);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	fp = _popen(command, "r");
#else
	fp = popen(command, "r");
#endif

	if (fp != NULL) {
		if (fgets(buffer, sizeof(buffer), fp) != NULL) {
			if (strlen(buffer) && buffer[strlen(buffer) - 1] == '\n') {
				buffer[strlen(buffer) - 1] = '\0';
			}

			text_buffer * content = buffer_filename(buffer, 1024);
			text_buffer_append_text(out, content->text, content->len);

			text_buffer_free(content, 1);
		}

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
		_pclose(fp);
#else
		pclose(fp);
#endif
	}
}


/// Print each line as is
static void export_latex_line(mmd_node * n, const char * text, text_buffer * out) {
	switch (n->type) {
		case LINE_SETEXT_1:
		case LINE_SETEXT_2:
			break;

		default:
			if (n->next) {
				text_buffer_append_text(out, &text[n->start], (int)n->len);
			} else {
				// Don't print final trailing newline
				text_buffer_append_text(out, &text[n->start], (int)n->len - 1);
			}

			break;
	}
}


static void export_latex_lines(mmd_node * n, const char * text, text_buffer * out) {
	while (n) {
		export_latex_line(n, text, out);

		n = n->next;
	}
}


/// Print each line as is, but only the "content" portion of each line
static void export_latex_line_content(mmd_line_node * l, const char * text, text_buffer * out) {
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
			} else if (l->c_len > 1) {
				// Don't print final trailing newline
				text_buffer_append_text(out, text, (int)l->c_len - 1);
			}

			break;
	}
}


static void export_latex_lines_content(mmd_node * n, const char * text, text_buffer * out) {
	while (n) {
		export_latex_line_content((mmd_line_node *) n, text, out);

		n = n->next;
	}
}


static void export_latex_raw_text(const char * text, size_t len, text_buffer * out) {
	const char * stop = text + len;

	while (text < stop) {
		switch (*text) {
			case '&':
				mmd_print_const(out, "\\&");
				break;

			case '#':
				mmd_print_const(out, "\\#");
				break;

			case '{':
				mmd_print_const(out, "\\{");
				break;

			case '}':
				mmd_print_const(out, "\\}");
				break;

			case '-':
				if (*(text + 1) == '-') {
					mmd_print_const(out, "-{}");
				} else {
					text_buffer_append_c(out, '-');
				}

				break;

			case '_':
				mmd_print_const(out, "\\_");
				break;

			case '$':
				mmd_print_const(out, "\\$");
				break;

			case '%':
				mmd_print_const(out, "\\%");
				break;

			case '\\':
				mmd_print_const(out, "\\textbackslash{}");
				break;

			case '~':
				mmd_print_const(out, "\\ensuremath{\\sim}");
				break;

			case '^':
				mmd_print_const(out, "\\^{}");
				break;

			case '\r':
				break;

			default:
				text_buffer_append_c(out, *text);
				break;
		}

		text++;
	}
}


static void export_latex_line_raw_content(mmd_line_node * l, const char * text, text_buffer * out) {
	text += l->general.start + l->c_start;

	switch (l->general.type) {
		case CODE_FENCE_LINE:
			break;

		case LINE_EMPTY:
			text_buffer_append_c(out, '\n');
			break;

		default:
			export_latex_raw_text(text, l->c_len, out);
			break;
	}
}


static void export_latex_lines_raw_content(mmd_line_node * l, const char * text, text_buffer * out) {
	while (l) {
		export_latex_line_raw_content(l, text, out);

		l = (mmd_line_node *) l->general.next;
	}
}


static void export_latex_line_verbatim_content(mmd_line_node * l, const char * text, text_buffer * out) {
	// text += l->general.start + l->c_start;
	text += l->general.start;

	// text_buffer_append_printf(out, "Verbatim type %d\n", l->general.type);

	switch (l->general.type) {
		case CODE_FENCE_LINE:
			break;

		case LINE_EMPTY:
			text_buffer_append_c(out, '\n');
			break;

		case LINE_INDENTED_TAB:
		case LINE_INDENTED_SPACE:
			text_buffer_append_text(out, &text[l->c_start], l->c_len);
			text_buffer_fix_trailing_newline(out);
			break;

		default:
			// text_buffer_append_text(out, text, l->c_len);
			text_buffer_append_text(out, text, l->general.len);
			text_buffer_fix_trailing_newline(out);
			break;
	}
}


static void export_latex_lines_verbatim_content(mmd_line_node * l, const char * text, text_buffer * out) {
	while (l) {
		export_latex_line_verbatim_content(l, text, out);

		l = (mmd_line_node *) l->general.next;
	}
}


/// Core function to export link_def
static int export_latex_link_def(link_def * l, const char * link_text, size_t link_text_len, mmd_node * link_text_token, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	if (l) {
		if (l->url[0] == '#') {
			export_latex_tokens(link_text_token, link_text, link_text_len, out, r, w, options);

			mmd_print_const(out, " (\\autoref{");

			url_encode_text(&l->url[1], l->url_len - 1, out);

			mmd_print_const(out, "})");
		} else {
			mmd_print_const(out, "\\href{");
			url_encode_text(l->url, l->url_len, out);
			mmd_print_const(out, "}{");
			export_latex_tokens(link_text_token, link_text, link_text_len, out, r, w, options);
			mmd_print_const(out, "}");

			// Include footnote with printed URL
			mmd_print_const(out, "\\footnote{\\href{");
			url_encode_text(l->url, l->url_len, out);
			mmd_print_const(out, "}{");
			export_latex_raw_text(l->url, l->url_len, out);
			mmd_print_const(out, "}}");
		}

		return 0;
	}

	return 1;
}


static int export_link_def_image(link_def * l, const char * link_text, size_t link_text_len, mmd_node * link_text_token, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options, bool is_figure) {
	if (!l) {
		return 1;
	}

	if (is_figure) {
		mmd_print_const(out, "\\begin{figure}[htbp]\n\\centering\n");
	}

	if (link_text_token) {

	}

	if (l->title_len) {

	}

	mmd_print_const(out, "\\includegraphics[");

	attr * a = l->attributes;

	char * height = NULL;
	char * width = NULL;

	while (a) {
		if (!strncmp("width", a->key, 5)) {
			width = a->key;

			if (!strncmp("px", &a->value[strlen(a->value) - 2], 2)) {
				text_buffer_append_printf(out, "%s=%.*spt", a->key, strlen(a->value) - 2, a->value);
			} else {
				text_buffer_append_printf(out, "%s=%s", a->key, a->value);
			}
		} else if (!strncmp("height", a->key, 6)) {
			height = a->key;

			if (!strncmp("px", &a->value[strlen(a->value) - 2], 2)) {
				text_buffer_append_printf(out, "%s=%.*spt", a->key, strlen(a->value) - 2, a->value);
			} else {
				text_buffer_append_printf(out, "%s=%s", a->key, a->value);
			}
		} else {
			text_buffer_append_printf(out, "%s=%s", a->key, a->value);
		}

		if (a->next) {
			mmd_print_const(out, ",");
		}

		a = a->next;
	}

	if (height || width) {
		if (!height || !width) {
			mmd_print_const(out, ",keepaspectratio,");

			if (!width) {
				mmd_print_const(out, "width=\\textwidth");
			}

			if (!height) {
				mmd_print_const(out, "height=0.75\\textheight");
			}
		}
	} else {
		// No dimensions specified, use sensible defaults
		mmd_print_const(out, "keepaspectratio,width=\\textwidth,height=0.75\\textheight");
	}

	mmd_print_const(out, "]{");

	if (l->url) {
		text_buffer_append_text(out, l->url, l->url_len);
	}

	mmd_print_const(out, "}");

	if (is_figure) {
		mmd_print_const(out, "\n");

		if (l->title_len) {
			mmd_print_const(out, "\\caption[");
			text_buffer_append_text(out, l->title, l->title_len);
			mmd_print_const(out, "]{");

			if (link_text_token) {
				export_latex_tokens(link_text_token, link_text, link_text_len, out, r, w, options);
			}

			mmd_print_const(out, "}\n");
		} else if (link_text_token) {
			mmd_print_const(out, "\\caption{");
			export_latex_tokens(link_text_token, link_text, link_text_len, out, r, w, options);
			mmd_print_const(out, "}\n");
		}

		if (l->key) {
			text_buffer_append_printf(out, "\\label{%s}\n", l->key);
		}

		mmd_print_const(out, "\\end{figure}");
	}

	return 0;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static int export_abbr_def(abbr_def * a, const char * link_text, size_t link_text_len, mmd_node * key_token, mmd_node * expansion_token, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options, bool * first) {
	if (a) {
		if (*first) {
			mmd_print_const(out, "\\newacronym{");
			export_latex_raw_text(&a->key[1], strlen(a->key) - 1, out);
			mmd_print_const(out, "}{");
			export_latex_raw_text(&a->key[1], strlen(a->key) - 1, out);
			mmd_print_const(out, "}{");

			if (expansion_token) {
				export_latex_tokens(expansion_token, link_text, link_text_len, out, r, w, options);
			} else {
				export_latex_raw_text(a->expansion, a->expansion_len, out);
			}

			mmd_print_const(out, "}");
		}

		mmd_print_const(out, "\\gls{");

		export_latex_raw_text(&a->key[1], strlen(a->key) - 1, out);
		mmd_print_const(out, "}");


		*first = !a->used;

		a->used = true;

		return 0;
	}

	return 1;
}
#pragma GCC diagnostic pop


static int export_latex_abbreviation_word(const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options, bool * first) {
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
		stack * s = NULL;

		switch (type) {
			case TOKEN_PAIR_BRACKET_CITATION:
				s = w->used_cite_stack;
				break;

			case TOKEN_PAIR_BRACKET_FOOTNOTE:
				s = w->used_note_stack;
				break;

			case TOKEN_PAIR_BRACKET_GLOSSARY:
				s = w->used_glos_stack;
				break;
		}

		// Update index
		if (e->index == 0) {
			// Endnote has not been used previously

			// Add to appropriate used endnote stack
			stack_push(s, e);

			// Update index
			e->index = (int) s->size;
		}

		switch (type) {
			case TOKEN_PAIR_BRACKET_CITATION: {
				// \citet for textual citations (e.g. author in text)
				// \citep for parenthetical citations (e.g. author in parentheses)
				if (w->in_endnote) {
					mmd_print_const(out, "\\bibitem");
				} else if (e->not_cited) {
					mmd_print_const(out, "~\\nocite");
				} else {
					if (e->citet) {
						mmd_print_const(out, "\\citet");
					} else {
						mmd_print_const(out, "~\\citep");
					}
				}

				if (t && t->len) {
					mmd_print_const(out, "[");
					export_latex_tokens(t, text, len, out, r, w, options);
					mmd_print_const(out, "]");
				}

				if (read_ctx_get_meta(r, "bibtex")) {
					mmd_print_const(out, "{");

					if (e->content_node && MMD_NODE_IS_BLOCK(e->content_node)) {
						export_latex_blocks(e->content_node, e->text, out, r, w, options);
					} else {
						export_latex_tokens(e->content_node, e->text, e->len, out, r, w, options);
					}

					// Trim ';' if present
					if (out->text[out->len - 1] == ';') {
						out->len--;
						out->text[out->len] = '\0';
					}

					mmd_print_const(out, "}");
				} else {
					char * id = html_id_from_text(&e->key[1], strlen(e->key) - 1, false);
					text_buffer_append_printf(out, "{%s}", id);
					free(id);
				}

				if (w->in_endnote) {
					mmd_print_const(out, "\n");
				}
			}
			break;

			case TOKEN_PAIR_BRACKET_FOOTNOTE:

				// Don't allow nested footnotes -- can lead to infinite loop
				if (w->in_endnote == 0) {
					mmd_print_const(out, "\\footnote{");
					w->padding = 2;

					w->in_endnote = 1;

					if (!e->is_inline) {
						w->skip_endnote_label = true;
					}

					if (e->content_node && MMD_NODE_IS_BLOCK(e->content_node)) {
						export_latex_blocks(e->content_node, e->text, out, r, w, options);
					} else {
						export_latex_tokens(e->content_node, e->text, e->len, out, r, w, options);
					}

					mmd_print_const(out, "}");

					w->in_endnote = 0;
					w->skip_endnote_label = false;
				}

				break;

			case TOKEN_PAIR_BRACKET_GLOSSARY: {
				if (e->is_inline) {
					// Inline glossary definition

					// Called by raw text key from title
					mmd_print_const(out, "\\newglossaryentry{");

					if (t) {
						if (t->type == TOKEN_PAIR_PAREN && t->child && t->next) {
							export_latex_raw_text(&text[t->child->start], t->next->start - t->child->start, out);
						} else {
							export_latex_raw_text(&text[t->start], t->tail->start + t->tail->len - t->start, out);
						}
					}

					// Name is formatted MultiMarkdown
					mmd_print_const(out, "}{name=");
					export_latex_tokens(e->title_node, e->text, e->len, out, r, w, options);

					// Description is formatted MultiMarkdown
					mmd_print_const(out, ", description={");

					w->padding = 2;
					w->in_endnote = 1;

					if (!e->is_inline) {
						w->skip_endnote_label = true;
					}

					if (e->content_node && MMD_NODE_IS_BLOCK(e->content_node)) {
						export_latex_blocks(e->content_node, e->text, out, r, w, options);
					} else {
						export_latex_tokens(e->content_node, e->text, e->len, out, r, w, options);
						w->padding = 0;
					}

					w->in_endnote = 0;
					w->skip_endnote_label = false;

					mmd_print_const(out, "}}");
				} else {
					// Reference glossary definition
				}

				// Glossary entries referenced by key
				mmd_print_const(out, "\\gls{");

				if (t) {
					if ((t->type == TOKEN_PAIR_PAREN) && t->child && t->next) {
						export_latex_raw_text(&text[t->child->start], t->next->start - t->child->start, out);
					} else if ((t->type == TOKEN_GLOSSARY_MARKER) && t->next) {
						export_latex_raw_text(&text[t->next->start], t->tail->start + t->tail->len - t->next->start, out);
					} else {
						export_latex_raw_text(&text[t->start], t->tail->start + t->tail->len - t->start, out);
					}
				} else {
					export_latex_raw_text(&e->key[1], strlen(e->key) - 1, out);
				}

				mmd_print_const(out, "}");
			}
			break;
		}

		return 0;
	}

	return 1;
}


static int export_latex_glossary_word(const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
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
	&export_latex_raw_text,
	&export_latex_tokens,
	&export_latex_link_def,
	&export_link_def_image,
	&export_abbr_def,
	&export_endnote_def
};


static void export_latex_token(mmd_node ** t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
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
				mmd_print_const(out, "--");
			}

			break;

		case TOKEN_HTML_ENTITY:
			export_latex_raw_text(&text[(*t)->start], (*t)->len, out);
			break;

		case TOKEN_MATH_DOLLAR_SINGLE:
		case TOKEN_BRACE_LEFT:
		case TOKEN_BRACE_RIGHT:
			mmd_print_const(out, "\\");
			text_buffer_append_text(out, &text[(*t)->start], (*t)->len);
			break;

		case TOKEN_MATH_DOLLAR_DOUBLE:
			mmd_print_const(out, "\\$\\$");
			break;

		case TOKEN_PAIR_MATH_PAREN:
		case TOKEN_PAIR_MATH_BRACKET:
		case TOKEN_PAIR_MATH_DOLLAR_SINGLE:
		case TOKEN_PAIR_MATH_DOLLAR_DOUBLE:

			// Don't output math delimiters if math starts with "\begin"
			if (strncmp(&text[(*t)->start + (*t)->len], "\\begin", 6) != 0) {
				if (rule.prefix) {
					text_buffer_append_text(out, rule.prefix, rule.prefix_len);
					w->padding = rule.pre_descent_padding;
				}
			}

			if ((*t)->child) {
				text_buffer_append_text(out, &text[(*t)->child->start], (*t)->child->tail->start + (*t)->child->tail->len - (*t)->child->start);
			}


			if (strncmp(&text[(*t)->start + (*t)->len], "\\begin", 6) != 0) {
				if (rule.suffix) {
					text_buffer_append_text(out, rule.suffix, rule.suffix_len);
				}
			}

			F(i, rule.skip) {
				(*t) = (*t)->next;
			}

			w->padding = 0;
			break;

		case TOKEN_SPECIAL_CHARACTER:
			switch (text[(*t)->start]) {
				case '\\':
					mmd_print_const(out, "\\textbackslash{}");
					break;

				case '%':
					mmd_print_const(out, "\\%");
					break;

				default:
					text_buffer_append_c(out, text[(*t)->start + 1]);
					break;
			}

			break;

		case TOKEN_PIPE:
			F(i, (int)(*t)->len) {
				mmd_print_const(out, "\\textbar{}");
			}
			break;

		case TOKEN_ESCAPED_CHARACTER:

			//export_latex_raw_char(text[(*t)->start + 1], out);
			switch (text[(*t)->start + 1]) {
				case '#':
					mmd_print_const(out, "\\#");
					break;

				case '~':
					mmd_print_const(out, "\\ensuremath{\\sim}");
					break;

				case '\\':
					mmd_print_const(out, "\\textbackslash{}");
					break;

				case '|':
					mmd_print_const(out, "\\textbar{}");
					break;

				case '^':
					mmd_print_const(out, "\\^{}");
					break;

				case '<':
					mmd_print_const(out, "$<$");
					break;

				case '>':
					mmd_print_const(out, "$>$");
					break;

				case '{':
				case '}':
				case '$':
				case '%':
				case '&':
				case '_':

					mmd_print_const(out, "\\");

				default:
					text_buffer_append_c(out, text[(*t)->start + 1]);
					break;
			}

			break;

		case TOKEN_HASH:
			F(i, (int)(*t)->len) {
				mmd_print_const(out, "\\#");
			}
			break;

		case TOKEN_TAG:
			mmd_print_const(out, "\\");
			text_buffer_append_text(out, &text[(*t)->start], (*t)->len);
			break;

		case TOKEN_LINEBREAK:
			if ((*t)->next) {
				mmd_print_const(out, "\\\\\n");
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

			// mmd_print_const(out, "\\gls{");
			// text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
			// mmd_print_const(out, "}");

			if (export_latex_abbreviation_word(&text[(*t)->start], (*t)->len, out, r, w, options, &first)) {
				text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
			} else {
				// mmd_print_const(out, "B");
				// mmd_print_const(out, "\\gls{");
				// text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
				// mmd_print_const(out, "}");
			}
		}
		break;

		case TOKEN_TEXT_GLOSSARY:
			if (export_latex_glossary_word(&text[(*t)->start], (*t)->len, out, r, w, options)) {
				text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
			}

			break;

		case TOKEN_PAIR_CM_SUB_DEL:
			mmd_print_const(out, "\\sout{");
			export_latex_tokens((*t)->child, text, len, out, r, w, options);
			mmd_print_const(out, "}");

			if ((*t)->next && (*t)->next->type == TOKEN_PAIR_CM_SUB_ADD) {
				(*t) = (*t)->next;
				mmd_print_const(out, "\\underline{");
				export_latex_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "}");
				(*t) = (*t)->next;
			}

			break;

		case TOKEN_PAIR_QUOTE_DOUBLE:
			text_buffer_append_text(out, double_quotes[(int)(int)r->quotes_language].opener, double_quotes[(int)(int)r->quotes_language].opener_len);
			export_latex_tokens((*t)->child, text, len, out, r, w, options);
			text_buffer_append_text(out, double_quotes[(int)(int)r->quotes_language].closer, double_quotes[(int)(int)r->quotes_language].closer_len);
			(*t) = (*t)->next;
			break;

		case TOKEN_PAIR_QUOTE_SINGLE:
			text_buffer_append_text(out, single_quotes[(int)(int)r->quotes_language].opener, single_quotes[(int)(int)r->quotes_language].opener_len);
			export_latex_tokens((*t)->child, text, len, out, r, w, options);
			text_buffer_append_text(out, single_quotes[(int)(int)r->quotes_language].closer, single_quotes[(int)(int)r->quotes_language].closer_len);
			(*t) = (*t)->next;
			break;

		case TOKEN_PAIR_SUPERSCRIPT:
			mmd_print_const(out, "\\textsuperscript{");
			export_latex_tokens((*t)->child, text, len, out, r, w, options);
			mmd_print_const(out, "}");

			if ((*t)->next && (*t)->next->type == TOKEN_SUPERSCRIPT) {
				(*t) = (*t)->next;
			}

			break;

		case TOKEN_SUPERSCRIPT:
			mmd_print_const(out, "\\");
			text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);
			mmd_print_const(out, "{}");
			break;

		case TOKEN_PAIR_SUBSCRIPT:
			mmd_print_const(out, "\\textsubscript{");
			export_latex_tokens((*t)->child, text, len, out, r, w, options);
			mmd_print_const(out, "}");

			if ((*t)->next && (*t)->next->type == TOKEN_SUBSCRIPT) {
				(*t) = (*t)->next;
			}

			break;

		case TOKEN_SUBSCRIPT:
			F(i, (int)(*t)->len) {
				mmd_print_const(out, "\\ensuremath{\\sim}");
			}
			break;

		case TOKEN_PAIR_BACKTICK:
			if (!(options & MMD_OPTION_COMPATIBILITY)) {
				if ((*t)->next && (*t)->next->next && ((*t)->next->next->type == TOKEN_PAIR_BRACE)) {
					mmd_node * n = (*t)->next->next;

					if (strncmp(&text[n->start], "{=", 2) == 0) {
						if (raw_filter_matches_format(&text[n->start], FORMAT_LATEX)) {
							// Raw text that should be included for LaTeX format

							if ((*t)->child) {
								text_buffer_append_text(out, &text[(*t)->child->start], (*t)->child->tail->start + (*t)->child->tail->len - (*t)->child->start);
							}

						}

						(*t) = n->next;
						break;
					}
				}
			}

			text_buffer_append_text(out, rule.prefix, rule.prefix_len);

			if ((*t)->child) {
				export_latex_raw_text(&text[(*t)->child->start], (*t)->child->tail->start + (*t)->child->tail->len - (*t)->child->start, out);
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
					text_buffer_append_text(out, &text[(*t)->start], (int)(*t)->len);

					switch ((*t)->type) {
						case TOKEN_PAIR_BRACKET_VARIABLE:
							mmd_print_const(out, "\\");

						case TOKEN_PAIR_BRACKET_ABBREVIATION:
						case TOKEN_PAIR_BRACKET_CITATION:
						case TOKEN_PAIR_BRACKET_FOOTNOTE:
						case TOKEN_PAIR_BRACKET_GLOSSARY:
							// We'll need to print this character
							(*t)->child->type = TOKEN_TEXT;
							break;

						default:
							break;
					}

					export_latex_tokens((*t)->child, text, len, out, r, w, options);
				} else {
//                    if (first) {
//                        mmd_print_const(out, "}");
//                    } else {
//                        mmd_print_const(out, a);
//                    }

				}
			}

			break;

		case TOKEN_PAIR_ANGLE:
			if ((*t)->child && scan_url(&text[(*t)->child->start]) == (*t)->next->start - (*t)->start - (*t)->len) {
				// This is an "automatic" link, e.g. <http://...>
				mmd_print_const(out, "\\href{");
				export_latex_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "}{");
				export_latex_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "}");
				(*t) = (*t)->next;
			} else if ((*t)->child && scan_email(&text[(*t)->child->start]) == (*t)->next->start - (*t)->start - (*t)->len) {
				// This is an "automatic" mailto, e.g. <mailto:...>
				mmd_print_const(out, "\\href{");

				if (strncmp("mailto:", &text[(*t)->child->start], 7)) {
					mmd_print_const(out, "mailto:");
				}

				export_latex_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "}{");
				export_latex_tokens((*t)->child, text, len, out, r, w, options);
				mmd_print_const(out, "}");
				(*t) = (*t)->next;
			} else if (scan_html(&text[(*t)->start])) {
				// Ignore HTML in LaTeX output
				// text_buffer_append_text(out, &text[(*t)->start], (int)((*t)->next->start + (*t)->next->len - (*t)->start));
				(*t) = (*t)->next;
			} else {
				// This is plain text that happens to be wrapped in <...>
				// <html> is handled elsewhere
				mmd_print_const(out, "<");
				export_latex_tokens((*t)->child, text, len, out, r, w, options);
			}

			break;

		case TOKEN_TABLE_CELL:

			// Handle colspan
			if ((*t)->next && (*t)->next->type == TOKEN_TABLE_DIVIDER) {
				if ((*t)->next->len > 1) {
					text_buffer_append_printf(out, "\\multicolumn{%d}{", (*t)->next->len);

					// TODO: Handle alignment
					// fprintf(stderr, "colspan %d '%c'\n", w->table_cell_count, w->table_alignment[w->table_cell_count]);
					// text_buffer_append_printf(out, "colspan %d/%d '%c'\n", w->table_cell_count, w->table_col_count, w->table_alignment[w->table_cell_count]);
					if (w->table_cell_count < w->table_col_count) {
						switch (w->table_alignment[w->table_cell_count]) {
							case 'l':
							case 'L':
								mmd_print_const(out, "l}{");
								break;

							case 'r':
							case 'R':
								mmd_print_const(out, "r}{");
								break;

							default:
								mmd_print_const(out, "c}{");
								break;
						}
					} else {
						mmd_print_const(out, "l}{");
					}
				}
			}

			// Print cell contents
			export_latex_tokens((*t)->child, text, (*t)->len, out, r, w, options);

			if ((*t)->next && (*t)->next->type == TOKEN_TABLE_DIVIDER) {
				if ((*t)->next->len > 1) {
					mmd_print_const(out, "}");
					w->table_cell_count += (*t)->next->len;
				}

				mmd_node * n = (*t)->next;

				if (n && n->next && n->next->type == TOKEN_TABLE_CELL) {
					mmd_print_const(out, "&");
				}
			} else {
				w->table_cell_count++;
			}

			break;

		default:
			if (rule.prefix) {
				text_buffer_append_text(out, rule.prefix, rule.prefix_len);
				w->padding = rule.pre_descent_padding;
			}

			if (rule.descent & DESCEND_CHILD) {
				if ((*t)->child) {
					export_latex_tokens((*t)->child, text, len, out, r, w, options);
				}
			}

			if (rule.descent & DESCEND_CONTENT) {
			}

			if (rule.descent & DESCEND_RAW) {
				if ((*t)->child) {
					export_latex_raw_text(&text[(*t)->child->start], (*t)->child->tail->start + (*t)->child->tail->len - (*t)->child->start, out);
				}
			}

			if (rule.descent & DESCEND_VERBATIM) {
				if ((*t)->child) {
					text_buffer_append_text(out, &text[(*t)->child->start], (*t)->child->tail->start + (*t)->child->tail->len - (*t)->child->start);
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


static void export_latex_tokens(mmd_node * t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	while (t) {
		export_latex_token(&t, text, len, out, r, w, options);

		t = t->next;
	}
}


static void export_latex_toc(const char * text, text_buffer * out) {
	int min = 0;
	int max = 6;

	scan_toc_range(text, &min, &max);

	if (max < 6) {
		text_buffer_append_printf(out, "\\setcounter{tocdepth}{%d}\n", max - 2);
	}

	mmd_print_const(out, "\\tableofcontents");
}


static void export_latex_block(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
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
					if (raw_filter_matches_format(lang, FORMAT_LATEX)) {
						w->padding = 0;

						if (w->in_recursive) {
							export_latex_lines_raw_content((mmd_line_node *)b->child, &text[b->start], out);
						} else {
							export_latex_lines_verbatim_content((mmd_line_node *)b->child, &text[b->start], out);
						}

						w->padding = 1;
					}

					free(lang);
					break;
				}

				text_buffer_append_printf(out, "\\begin{lstlisting}[language=%s]\n", lang);
				free(lang);
			} else {
				mmd_print_const(out, "\\begin{verbatim}\n");
			}

			w->padding = 1;

			if (w->in_recursive) {
				export_latex_lines_raw_content((mmd_line_node *)b->child, &text[b->start], out);
			} else {
				export_latex_lines_verbatim_content((mmd_line_node *)b->child, &text[b->start], out);
			}

			if (lang) {
				mmd_print_const(out, "\\end{lstlisting}\n");
			} else {
				mmd_print_const(out, "\\end{verbatim}\n");
			}

			w->padding = 1;
		}
		break;

		case BLOCK_DEFLIST:
			pad(out, 2, w);

			// Group consecutive definition lists together
			if (!w->in_definition_list) {
				mmd_print_const(out, "\\begin{description}\n");
			}

			w->padding = 2;

			export_latex_blocks(b->child, &text[b->start], out, r, w, options);
			pad(out, 1, w);

			if (b->next && b->next->type == BLOCK_DEFLIST) {
				w->in_definition_list = true;
			} else {
				w->in_definition_list = false;
				mmd_print_const(out, "\\end{description}\n");
			}

			w->padding = 1;
			break;

		case BLOCK_DEFINITION:
			pad(out, 2, w);

			if (b->child->next) {
				export_latex_blocks(b->child, &text[b->start], out, r, w, options);
			} else {
				export_latex_tokens(b->child->content, &text[b->start], b->len, out, r, w, options);
			}

			w->padding = 0;
			break;

		case BLOCK_H1:
		case BLOCK_H2:
		case BLOCK_H3:
		case BLOCK_H4:
		case BLOCK_H5:
		case BLOCK_H6: {
			pad(out, 2, w);

			int level = b->type - BLOCK_H1 + read_ctx_get_header_level(r, FORMAT_LATEX);

			if ((g_format == FORMAT_BEAMER) || (g_format == FORMAT_LTX_TALK)) {
				if (g_in_frame) {
					if (level < 5) {
						mmd_print_const(out, "\\end{frame}\n\n");
						g_in_frame = 0;
					}
				}

				if (level == 3) {
					g_in_frame = 1;
				}
			}

			if ((level >= 0) && (level < 6)) {
				text_buffer_append_text(out, headers[g_format][level].opener, headers[g_format][level].opener_len);
			}

			if (options & MMD_OPTION_COMPATIBILITY) {
				export_latex_tokens(b->content, &text[b->start], b->len, out, r, w, options);
				text_buffer_trim_trailing_whitespace(out);
			} else {
				export_latex_tokens(b->content, &text[b->start], b->len, out, r, w, options);
				text_buffer_trim_trailing_whitespace(out);

				mmd_print_const(out, "}\n\\label{");

				header * h = stack_peek_index(r->header_stack, w->header_count++);

				// TODO: The label should be ok as-is, and we don't want to escape underscores, for example
				if (h) {
					// export_latex_raw_text(h->key, strlen(h->key), out);
					text_buffer_append_text(out, h->key, strlen(h->key));
				} else {
					char * id = html_id_from_text(&text[b->start], b->len, true);
					// export_latex_raw_text(id, strlen(id), out);
					text_buffer_append_text(out, id, strlen(id));
					free(id);
				}
			}

			if ((level >= 0) && (level < 6)) {
				text_buffer_append_text(out, headers[g_format][level].closer, headers[g_format][level].closer_len);
			}

			w->padding = 0;
		}
		break;

		case BLOCK_SETEXT_1:
		case BLOCK_SETEXT_2: {
			pad(out, 2, w);

			int level = b->type - BLOCK_SETEXT_1 + read_ctx_get_header_level(r, FORMAT_LATEX);

			if ((g_format == FORMAT_BEAMER) || (g_format == FORMAT_LTX_TALK)) {
				if (g_in_frame) {
					if (level < 5) {
						mmd_print_const(out, "\\end{frame}\n\n");
						g_in_frame = 0;
					}
				}

				if (level == 3) {
					g_in_frame = 1;
				}
			}

			if ((level >= 0) && (level < 6)) {
				text_buffer_append_text(out, headers[g_format][level].opener, headers[g_format][level].opener_len);
			}

			export_latex_tokens(b->content, &text[b->start], b->len, out, r, w, options);
			text_buffer_trim_trailing_whitespace(out);

			if ((level >= 0) && (level < 6)) {
				text_buffer_append_text(out, headers[g_format][level].closer, headers[g_format][level].closer_len);
			}

			if (options & MMD_OPTION_COMPATIBILITY) {
			} else {
				header * h = stack_peek_index(r->header_stack, w->header_count++);

				mmd_print_const(out, "\n\\label{");

				if (h) {
					export_latex_raw_text(h->key, strlen(h->key), out);
				} else {
					char * id = html_id_from_text(&text[b->start], b->len - b->child->tail->len, true);
					export_latex_raw_text(id, strlen(id), out);
					free(id);
				}

				mmd_print_const(out, "}");
			}

			w->padding = 0;
		}
		break;

		case BLOCK_LIST_ITEM_TIGHT:
			// Custom because we need to handle the first child differently
			pad(out, 1, w);
			mmd_print_const(out, "\\item ");
			w->padding = 2;

			if (MMD_NODE_IS_BLOCK(b->child)) {
				if (b->child->type == BLOCK_PARA) {
					export_latex_tokens(b->child->content, &text[b->start], b->len, out, r, w, options);
				} else {
					export_latex_block(b->child, &text[b->start], out, r, w, options);
				}
			} else {
				export_latex_tokens(b->child->content, &text[b->start], b->len, out, r, w, options);
			}

			w->padding = 0;
			export_latex_blocks(b->child->next, &text[b->start], out, r, w, options);
			w->padding = 0;
			break;

		case BLOCK_PARA:
			pad(out, 2, w);
			export_latex_tokens(b->content, &text[b->start], b->len, out, r, w, options);

			w->padding = 0;
			break;

		case BLOCK_TOC:
			pad(out, 2, w);

			export_latex_toc(&text[b->start], out);

			w->padding = 0;
			break;

		case BLOCK_TABLE:
			pad(out, 2, w);

			mmd_print_const(out, "\\begin{table}[htbp]\n");
			mmd_print_const(out, "\\begin{minipage}{\\linewidth}\n\\setlength{\\tymax}{0.5\\linewidth}\n\\centering\n\\small\n");

			// Is there a caption?
			if (table_has_caption(b)) {
				char * id = table_label(b, text);

				mmd_node * label = b->next->content->next;

				if (label && label->next && label->next->type == TOKEN_PAIR_BRACKET) {
					text_buffer_append_printf(out, "\\caption[%s]{", id);
				} else {
					mmd_print_const(out, "\\caption{");
				}

				export_latex_tokens(b->next->content->child, &text[b->next->start], b->next->len, out, r, w, options);
				mmd_print_const(out, "}\n");
				text_buffer_append_printf(out, "\\label{%s}\n", id);
				free(id);
			}

			// Handle column setup and alignment
			write_ctx_get_table_alignments(w, b, &text[b->start]);

			w->padding = 2;

			// Export table rows
			export_latex_blocks(b->child, &text[b->start], out, r, w, options);
			pad(out, 1, w);

			mmd_print_const(out, "\n\\end{tabulary}\n\\end{minipage}");
			mmd_print_const(out, "\n\\end{table}");

			w->padding = 0;

			if (table_has_caption(b)) {
				// Don't export capition again
				w->skip_blocks = 1;
			}

			break;

		case BLOCK_TABLE_HEADER:
			pad(out, 2, w);

			text_buffer_append_text(out, rule.prefix, rule.prefix_len);
			w->padding = rule.pre_descent_padding;

			mmd_print_const(out, "{@{}");
			F(i, w->table_col_count) {
				switch (w->table_alignment[i]) {
					case 'l':
					case 'L':
					case 'r':
					case 'R':
					case 'c':
					case 'C':
						text_buffer_append_c(out, w->table_alignment[i]);
						break;

					case 'N':
						text_buffer_append_c(out, 'L');
						break;

					default:
						text_buffer_append_c(out, 'l');
						break;
				}
			}

			mmd_print_const(out, "@{}} \\toprule\n");
			w->padding = 2;

			w->in_table_header = true;
			export_latex_blocks(b->child, &text[b->start], out, r, w, options);
			w->in_table_header = false;

			pad(out, rule.pad_post_descent, w);

			text_buffer_append_text(out, rule.suffix, rule.suffix_len);

			w->padding = rule.post_suffix_padding;
			break;

		case BLOCK_TABLE_SEPARATOR:
			if (w->in_table_header) {
				break;
			}

		default:
			pad(out, rule.padding, w);

			if (rule.prefix) {
				text_buffer_append_text(out, rule.prefix, rule.prefix_len);
				w->padding = rule.pre_descent_padding;
			}

			switch (b->type) {
				case BLOCK_TABLE_ROW:
					w->table_cell_count = 0;
					break;

				case BLOCK_FIGURE:
					w->in_figure = true;
					break;

				case BLOCK_TABLE_HEADER:
					w->table_col_count = 0;
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
				export_latex_blocks(b->child, &text[b->start], out, r, w, options);
			}

			if (rule.descent & DESCEND_CONTENT) {
				export_latex_tokens(b->content, &text[b->start], b->len, out, r, w, options);
			}

			if (rule.descent & DESCEND_LINES) {
				if (w->in_recursive) {
					export_latex_lines_content(b->child, &text[b->start], out);
				} else {
					export_latex_lines(b->child, &text[b->start], out);
				}
			}

			if (rule.descent & DESCEND_LINES_RAW) {
				export_latex_lines_raw_content((mmd_line_node *)b->child, &text[b->start], out);
			}

			if (rule.descent & DESCEND_LINES_VERBATIM) {
				export_latex_lines_verbatim_content((mmd_line_node *)b->child, &text[b->start], out);
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


static void export_latex_blocks(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	while (b) {
		if (w->skip_blocks) {
			w->skip_blocks--;
		} else {
			export_latex_block(b, text, out, r, w, options);
		}

		b = b->next;
	}
}


static int abbr_key_sort(abbr_def * a, abbr_def * b) {
	return strcmp(a->key, b->key);
}


static int endnote_key_sort(endnote_def * a, endnote_def * b) {
	return strcmp(a->key, b->key);
}


static void export_latex_glossary(text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	// Iterate through glossary definitions
	endnote_def * e, * e_tmp;

	// Sort alphabetically by key
	HASH_SORT(r->glos_def_hash, endnote_key_sort);

	HASH_ITER(hh, r->glos_def_hash, e, e_tmp) {
		// Called by raw text key from title
		mmd_print_const(out, "\\longnewglossaryentry{");
		export_latex_raw_text(&e->key[1], strlen(e->key) - 1, out);

		// Name is formatted MultiMarkdown
		mmd_print_const(out, "}{name=");
		export_latex_tokens(e->title_node, e->text, e->len, out, r, w, options);

		// Description is formatted MultiMarkdown
		mmd_print_const(out, "}{");
		w->padding = 2;

		w->in_endnote = 1;

		if (!e->is_inline) {
			w->skip_endnote_label = true;
		}

		if (e->content_node && MMD_NODE_IS_BLOCK(e->content_node)) {
			export_latex_blocks(e->content_node, e->text, out, r, w, options);
		} else {
			export_latex_tokens(e->content_node, e->text, e->len, out, r, w, options);
			w->padding = 0;
		}

		w->in_endnote = 0;
		w->skip_endnote_label = false;

		mmd_print_const(out, "}\n");
	}


	// And abbreviations
	abbr_def * a, * a_tmp;

	// Sort alphabetically by key
	HASH_SORT(r->abbr_def_hash, abbr_key_sort);

	HASH_ITER(hh, r->abbr_def_hash, a, a_tmp) {
		mmd_print_const(out, "\\newacronym{");
		export_latex_raw_text(&a->key[1], strlen(a->key) - 1, out);
		mmd_print_const(out, "}{");
		export_latex_raw_text(&a->key[1], strlen(a->key) - 1, out);
		mmd_print_const(out, "}{");
		export_latex_raw_text(a->expansion, a->expansion_len, out);
		mmd_print_const(out, "}\n");
	}
}


static void export_metadata_text(text_buffer * out, const char * text, int len) {
	F(i, len) {
		switch (text[i]) {
			case '\n':
				text_buffer_append_text(out, "\\\n", 2);
				break;

			default:
				text_buffer_append_c(out, text[i]);
				break;
		}
	}
}


static void export_latex_header(text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	meta * m;

	m = read_ctx_get_meta(r, "documentmetadata");

	if (m) {
		text_buffer_append_printf(out, "\\DocumentMetadata{%s}\n", m->value);
	}

	m = read_ctx_get_meta(r, "latexleader");

	if (m) {
		if (options & MMD_OPTION_EMBED_ASSETS) {
			embed_input_file(out, m->value);
		} else {
			text_buffer_append_printf(out, "\\input{%s}\n", m->value);
		}
	} else {
		m = read_ctx_get_meta(r, "latexconfig");

		if (m) {
			if (options & MMD_OPTION_EMBED_ASSETS) {
				char buf[1024];
				snprintf(buf, sizeof(buf), "mmd7-%s-leader", m->value);
				embed_input_file(out, buf);
			} else {
				text_buffer_append_printf(out, "\\input{mmd7-%s-leader}\n", m->value);
			}
		} else {
			m = read_ctx_get_meta(r, "latexclass");

			if (m) {
				char * stop = strstr(m->value, "]");

				if (m->value[0] == '[' && stop) {
					// We have options
					mmd_print_const(out, "\\documentclass");
					text_buffer_append_text(out, m->value, stop - m->value);
					text_buffer_append_printf(out, "]{%s}\n", stop + 1);
				} else {
					text_buffer_append_printf(out, "\\documentclass{%s}\n", m->value);
				}
			}
		}
	}

	// Iterate over metadata keys
	for (m = r->meta_hash; m != NULL; m = m->hh.next) {
		switch (m->key[0]) {
			case 'a':
				if (strcmp(m->key, "author") == 0) {
					mmd_print_const(out, "\\def\\myauthor");
					mmd_print_const(out, "{");
					export_metadata_text(out, m->value, (int)m->value_len);
					mmd_print_const(out, "}\n");
					continue;
				} else if (strcmp(m->key, "address") == 0) {
					mmd_print_const(out, "\\def\\myaddress");
					mmd_print_const(out, "{");
					export_metadata_text(out, m->value, (int)m->value_len);
					mmd_print_const(out, "}\n");
					continue;
				}


				break;

			case 'b':

				if (strcmp(m->key, "baseheaderlevel") == 0) {
					continue;
				} else if (strcmp(m->key, "bibtex") == 0) {
					mmd_print_const(out, "\\def\\bibliocommand{\\bibliography{");
					text_buffer_append_text(out, m->value, (int)m->value_len);
					mmd_print_const(out, "}}\n");
					continue;
				}

				break;

			case 'c':
				if (strcmp(m->key, "css") == 0) {
					continue;
				} else if (strcmp(m->key, "copyright") == 0) {
					mmd_print_const(out, "\\def\\mycopyright");
					mmd_print_const(out, "{");
					export_metadata_text(out, m->value, (int)m->value_len);
					mmd_print_const(out, "}\n");
					continue;
				}

				break;

			case 'd':
				if (strcmp(m->key, "date") == 0) {
					mmd_print_const(out, "\\def\\mydate");
					mmd_print_const(out, "{");
					export_metadata_text(out, m->value, (int)m->value_len);
					mmd_print_const(out, "}\n");
					continue;
				}

				break;

			case 'h':
				if (strcmp(m->key, "htmlfooter") == 0) {
					continue;
				} else if (strcmp(m->key, "htmlheader") == 0) {
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
					text_buffer_append_text(out, m->value, (int)m->value_len);
					text_buffer_append_c(out, '\n');
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

			case 'r':
				if (strcmp(m->key, "returnaddress") == 0) {
					mmd_print_const(out, "\\def\\myreturnaddress");
					mmd_print_const(out, "{");
					export_metadata_text(out, m->value, (int)m->value_len);
					mmd_print_const(out, "}\n");
					continue;
				}

				break;

			case 's':
				if (strcmp(m->key, "subtitle") == 0) {
					mmd_print_const(out, "\\def\\mysubtitle");
					mmd_print_const(out, "{");
					export_metadata_text(out, m->value, (int)m->value_len);
					mmd_print_const(out, "}\n");
					continue;
				}

				break;

			case 't':
				if (strcmp(m->key, "title") == 0) {
					mmd_print_const(out, "\\def\\mytitle{");
					export_latex_raw_text(m->value, m->value_len, out);
					mmd_print_const(out, "}\n");
					continue;
				} else if (strcmp(m->key, "transcludebase") == 0) {
					continue;
				}

				break;

			case 'x':
				if (strcmp(m->key, "xhtmlheader") == 0) {
					continue;
				} else if (strcmp(m->key, "xhtmlheaderlevel") == 0) {
					continue;
				}

				break;
		}

		// Any other metadata comes here
		mmd_print_const(out, "\\def\\");
		text_buffer_append_printf(out, "%s", m->key);
		mmd_print_const(out, "{");
		export_metadata_text(out, m->value, (int)m->value_len);
		mmd_print_const(out, "}\n");
	}

	// Include core package, if specified
	m = read_ctx_get_meta(r, "latexpackage");

	if (m) {
		char * stop = strstr(m->value, "]");

		if (m->value[0] == '[' && stop) {
			// We have options
			mmd_print_const(out, "\\usepackage");
			text_buffer_append_text(out, m->value, stop - m->value);
			text_buffer_append_printf(out, "]{%s}\n", stop + 1);
		} else {
			text_buffer_append_printf(out, "\\usepackage{%s}\n", m->value);
		}
	}

	// Define glossary/acronym entries in preamble
	export_latex_glossary(out, r, w, options);

	m = read_ctx_get_meta(r, "latexbegin");

	if (m) {
		if (options & MMD_OPTION_EMBED_ASSETS) {
			embed_input_file(out, m->value);
		} else {
			text_buffer_append_printf(out, "\\input{%s}\n", m->value);
		}
	} else {
		m = read_ctx_get_meta(r, "latexconfig");

		if (m) {
			if (options & MMD_OPTION_EMBED_ASSETS) {
				char buf[1024];
				snprintf(buf, sizeof(buf), "mmd7-%s-begin", m->value);
				embed_input_file(out, buf);
			} else {
				text_buffer_append_printf(out, "\\input{mmd7-%s-begin}\n", m->value);
			}
		} else {
			mmd_print_const(out, "\\begin{document}\n");
		}
	}

	w->padding = 1;
}


static void export_latex_footer(text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	pad(out, 1, w);

	if (g_format == FORMAT_BEAMER) {
		mmd_print_const(out, "\\mode<all>\n");
	}

	meta * m = read_ctx_get_meta(r, "latexfooter");

	if (m) {
		if (options & MMD_OPTION_EMBED_ASSETS) {
			embed_input_file(out, m->value);
		} else {
			text_buffer_append_printf(out, "\\input{%s}\n", m->value);
		}
	} else {
		m = read_ctx_get_meta(r, "latexconfig");

		if (m) {
			if (options & MMD_OPTION_EMBED_ASSETS) {
				char buf[1024];
				snprintf(buf, sizeof(buf), "mmd7-%s-footer", m->value);
				embed_input_file(out, buf);
			} else {
				text_buffer_append_printf(out, "\\input{mmd7-%s-footer}\n", m->value);
			}
		}
	}

	w->padding = 1;

	mmd_print_const(out, "\\end{document}");

	if (g_format == FORMAT_BEAMER) {
		mmd_print_const(out, "\\mode*");
	}

	w->padding = 0;
}


static void export_latex_bibliography(text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	if (w->used_cite_stack->size) {
		pad(out, 2, w);

		if (g_format == FORMAT_LTX_TALK) {
			mmd_print_const(out, "\\makereferences");
			w->padding = 0;
			w->in_endnote = 0;
			return;
		}

		if (g_format == FORMAT_BEAMER) {
			mmd_print_const(out, "\\begin{frame}[allowframebreaks]\n" \
							"\\frametitle{Bibliography}\n" \
							"\\def\\newblock{}\n" \
						   );
		}

		mmd_print_const(out, "\\begin{thebibliography}{0}\n");

		w->padding = 2;
		w->in_endnote = 1;

		F(i, (int)w->used_cite_stack->size) {
			endnote_def * e = stack_peek_index(w->used_cite_stack, i);

			if (e->content_node && MMD_NODE_IS_BLOCK(e->content_node)) {
				export_latex_blocks(e->content_node, e->text, out, r, w, options);
			} else {
				char * id = html_id_from_text(&e->key[1], strlen(e->key) - 1, false);
				pad(out, 2, w);
				text_buffer_append_printf(out, "\\bibitem{%s}\n", id);
				free(id);
				export_latex_tokens(e->content_node, e->text, e->len, out, r, w, options);
			}

			pad(out, 1, w);
		}

		pad(out, 2, w);
		mmd_print_const(out, "\\end{thebibliography}");

		if (g_format == FORMAT_BEAMER) {
			mmd_print_const(out, "\n\\end{frame}");
		}

		w->padding = 0;
		w->in_endnote = 0;
	}
}


void export_latex(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, uint32_t options, enum output_format format) {
	g_format = format;

	precalculate_rules(rules, sizeof(rules) / sizeof(rules[0]));
	precalculate_quotes(single_quotes, sizeof(single_quotes) / sizeof((single_quotes[0])));
	precalculate_quotes(double_quotes, sizeof(double_quotes) / sizeof(double_quotes[0]));

	precalculate_quotes(headers[FORMAT_LATEX], sizeof(headers[FORMAT_LATEX]) / sizeof(headers[FORMAT_LATEX][0]));
	precalculate_quotes(headers[FORMAT_BEAMER], sizeof(headers[FORMAT_BEAMER]) / sizeof(headers[FORMAT_BEAMER][0]));
	precalculate_quotes(headers[FORMAT_LTX_TALK], sizeof(headers[FORMAT_LTX_TALK]) / sizeof(headers[FORMAT_LTX_TALK][0]));

	write_ctx * w = write_ctx_new();

	if (r->write_complete || (r->has_meta && !r->write_snippet)) {
		export_latex_header(out, r, w, options);
	}

	export_latex_blocks(b, text, out, r, w, options);

	if ((g_format == FORMAT_BEAMER) || (g_format == FORMAT_LTX_TALK)) {
		if (g_in_frame) {
			pad(out, 1, w);
			mmd_print_const(out, "\\end{frame}\n\n");
		}
	}

	if (!read_ctx_get_meta(r, "bibtex")) {
		// Include custom bibliography if we are not using BibTeX
		export_latex_bibliography(out, r, w, options);
	}

	if (r->write_complete || (r->has_meta && !r->write_snippet)) {
		export_latex_footer(out, r, w, options);
	}

	pad(out, 1, w);
	write_ctx_free(w);
}
