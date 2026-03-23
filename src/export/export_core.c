/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file export_core.c

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
#include <string.h>

#include "mmd_node.h"
#include "read_ctx.h"
#include "char.h"
#include "text_buffer.h"
#include "write_ctx.h"
#include "mmd_span_parser.h"
#include "mmd_scanner.h"
#include "mmd_token_scanner.h"
#include "export_core.h"
#include "mmd_utilities.h"


#ifdef TEST
	#include "CuTest.h"
#endif

#define F(i,n) for(int i= 0;i<n;i++)

/// strndup not available on all platforms
char * mmd_strndup(const char * source, size_t * n) {
	if (source == NULL) {
		return NULL;
	}

	size_t len = 0;
	char * result;
	const char * test = source;

	// strlen is too slow if strlen(source) >> n
	for (len = 0; len < *n; ++len) {
		if (*test == '\0') {
			break;
		}

		test++;
	}

	result = malloc(len + 1);

	if (result) {
		memcpy(result, source, len);
		result[len] = '\0';
	}

	*n = len;
	return result;
}


/// Precalculate lengths of all constant strings
void precalculate_rules(parse_rule * rules, int n) {
	F(i, n) {
		if (rules[i].prefix) {
			rules[i].prefix_len = (int) strlen(rules[i].prefix);
		}

		if (rules[i].suffix) {
			rules[i].suffix_len = (int) strlen(rules[i].suffix);
		}
	}
}


/// Precalculate lengths of all smart quote strings
void precalculate_quotes(smart_quote * quotes, int n) {
	F(i, n) {
		if (quotes[i].opener) {
			quotes[i].opener_len = (int)strlen(quotes[i].opener);
		}

		if (quotes[i].closer) {
			quotes[i].closer_len = (int)strlen(quotes[i].closer);
		}
	}
}


void url_encode_text(const char * text, size_t len, text_buffer * out) {
	const char * stop = text + len;

	while (text < stop) {
		switch (*text) {
			case '\n':
			case '\r':
				// ignore these
				break;

			case '&':
				if (strncmp("&amp;", text, 5) == 0) {
					text += 4;
				}

				mmd_print_const(out, "&amp;");

				break;

			case '<':
				mmd_print_const(out, "&lt;");
				break;

			case '>':
				mmd_print_const(out, "&gt;");
				break;

			case ' ':
				mmd_print_const(out, "%20");
				break;

			case '"':
				mmd_print_const(out, "&quot;");
				break;

			case '\\':

				// Ignore unless there are two of them
				if (text[1] == '\\') {
					mmd_print_const(out, "%5C");
					text++;
				}

				break;

			default:
				text_buffer_append_c(out, *text);
				break;
		}

		text++;
	}
}


link_def * extract_inline_link(const char * text, size_t len, mmd_node ** t, uint32_t options) {
	mmd_node * link_text = (*t)->child;
	mmd_node * link_url = NULL;

	if (link_text) {
		link_url = (*t)->next->next;

		// Allow a single line break between the [link text] and the ()
		// Because line wrapping frequently hits us here
		if (link_url && link_url->type == TOKEN_NL) {
			if (link_url->next && link_url->next->type == TOKEN_PAIR_PAREN) {
				link_url = link_url->next;
			}
		}
	} else {
		// Empty content [](...)
		link_url = (*t)->next;

		if (link_url->type == TOKEN_BRACKET_RIGHT) {
			link_url = link_url->next;
		}
	}

	if ((link_url == NULL) || (link_url->next == NULL)) {
		return NULL;
	}

	// Extract link definition
	link_def * l = calloc(1, sizeof(link_def));

	const char * cur = &text[link_url->start + 1];
	const char * stop = &text[link_url->next->start];

	// Extract url
	text_buffer * url = text_buffer_new(128);

	// Skip leading whitespace
	while (cur < stop && char_is_whitespace_or_line_ending(*cur)) {
		cur++;
	}

	// Start url
	if (*cur == '<') {
		cur++;
	}

	// Build sanitized url
	while (cur < stop && !char_is_whitespace_or_line_ending(*cur) && *cur != '>') {
		switch (*cur) {
			case '\\':

				// Ignore backslash, unless there are two of them
				if (cur[1] == '\\') {
					text_buffer_append_text(url, "%5C", 3);
					cur++;
				}

				break;

			case '&':

				// Trim &amp; and replace with &, which will be output as needed
				if (strncmp("&amp;", cur, 5) == 0) {
					text_buffer_append_c(url, *cur);
					cur += 4;
				} else {
					text_buffer_append_c(url, *cur);
				}

				break;

			default:
				text_buffer_append_c(url, *cur);
				break;
		}

		cur++;
	}

	l->url = url->text;
	l->url_len = url->len;
	text_buffer_free(url, 0);

	if (*cur == '>') {
		cur++;
	}


	// Extract title

	// Skip leading whitespace
	while (cur < stop && char_is_whitespace_or_line_ending(*cur)) {
		cur++;
	}

	// Find the corresponding node
	mmd_node * title = link_url->child;

	while (title && title->next && (title->next->start <= (size_t)(cur - text))) {
		title = title->next;
	}

	if (title && title->next && title->start == (size_t)(cur - text)) {
		switch (title->type) {
			case TOKEN_PAIR_QUOTE_DOUBLE:
			case TOKEN_PAIR_QUOTE_SINGLE:
			case TOKEN_PAIR_PAREN:
				l->title_len = title->next->start - title->start - 1;
				l->title = my_strndup(&text[title->start + 1], l->title_len);
				cur = &text[title->next->start + 1];
				break;
		}
	}


	// Extract attributes
	if (!(options & MMD_OPTION_COMPATIBILITY)) {
		size_t scanned = 0;
		l->attributes = scan_link_attributes(cur, text + len - cur, &scanned);

		if (l->attributes) {
			cur += scanned;
		}
	}

	while (cur < stop && char_is_whitespace(*cur)) {
		cur++;
	}

	if (cur != stop) {
		link_def_free(l);
		l = NULL;
	}

	return l;
}


static int export_implicit_link(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {
	// [...] text and id are the same

	if ((*t)->child && (*t)->next) {
		char * id = md_id_from_text(&text[(*t)->child->start], (*t)->next->start - (*t)->child->start, true);
		// size_t id_len = (*t)->next->start - (*t)->child->start;
		// char * id = mmd_strndup(&text[(*t)->child->start], &id_len);
		link_def * l = read_ctx_get_link(r, id);
		free(id);

		if (l) {
			if (fe->export_link_def(l, text, len, (*t)->child, out, r, w, options)) {
				return 1;
			} else {
				// Skip over closer
				(*t) = (*t)->next;
				return 0;
			}
		}
	}

	return 1;
}


static int export_inline_link(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {
	// [...](...)
	link_def * l = extract_inline_link(text, len, t, options);

	if (l) {
		int res = fe->export_link_def(l, text, len, (*t)->child, out, r, w, options);
		link_def_free(l);

		if (!res) {
			switch ((*t)->type) {
				case TOKEN_PAIR_BRACKET_EMPTY:
					(*t) = (*t)->next;
					(*t) = (*t)->next;
					break;

				default:
					(*t) = (*t)->next;
					(*t) = (*t)->next;
					(*t) = (*t)->next;
					break;
			}
		}

		return res;
	}

	return 1;
}


static int export_split_link(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {
	// [...][...]  text and id are split
	mmd_node * id_node;

	if ((*t)->type == TOKEN_PAIR_BRACKET_EMPTY) {
		id_node = (*t)->next;
	} else {
		id_node = (*t)->next->next;
	}

	if (id_node->child && id_node->next) {
		char * id = md_id_from_text(&text[id_node->child->start], id_node->next->start - id_node->child->start, true);
		link_def * l = read_ctx_get_link(r, id);
		free(id);

		if (l) {
			if (fe->export_link_def(l, text, len, (*t)->child, out, r, w, options)) {
				return 1;
			} else {
				// Skip over closer and definition name
				(*t) = id_node->next;
				return 0;
			}
		}
	}

	return 1;
}


static int export_implicit_image(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {
	// ![...] text and id are the same

	if ((*t)->child && (*t)->next) {
		//char * def_name = definition_name_from_text(&text[(*t)->child->start], (*t)->next->start - (*t)->child->start);
		char * id = md_id_from_text(&text[(*t)->child->start], (*t)->next->start - (*t)->child->start, true);
		link_def * l = read_ctx_get_link(r, id);
		free(id);

		if (l) {
			if (fe->export_link_def_image(l, text, len, (*t)->child, out, r, w, options, w->in_figure)) {
				return 1;
			} else {
				// Skip over closer
				(*t) = (*t)->next;
				return 0;
			}
		}
	}

	return 1;
}


static int export_inline_image(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {

	// ![...](...)
	link_def * l = extract_inline_link(text, len, t, options);

	if (l) {
		int res = fe->export_link_def_image(l, text, len, (*t)->child, out, r, w, options, w->in_figure);
		(*t) = (*t)->next;
		(*t) = (*t)->next;
		(*t) = (*t)->next;
		link_def_free(l);
		return res;
	}

	return 1;
}


static int export_split_image(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {
	// ![...][...]
	mmd_node * id_node;

	if ((*t)->type == TOKEN_PAIR_BRACKET_EMPTY) {
		id_node = (*t)->next;
	} else {
		id_node = (*t)->next->next;
	}

	if (id_node->child) {
		char * id = md_id_from_text(&text[id_node->child->start], id_node->next->start - id_node->child->start, true);
		link_def * l = read_ctx_get_link(r, id);
		free(id);

		if (l) {
			if (fe->export_link_def_image(l, text, len, (*t)->child, out, r, w, options, w->in_figure)) {
				return 1;
			} else {
				// Skip over closer and id
				(*t) = id_node->next;
				return 0;
			}
		}
	}

	return 1;
}


static int export_implicit_abbreviation(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {
	// [>...] or [>(...) ...]

	if ((*t)->child && (*t)->next) {
		char * id = md_id_from_text(&text[(*t)->child->start], (*t)->next->start - (*t)->child->start, true);
		abbr_def * a = read_ctx_get_abbr(r, id);

		bool first = false;

		if (a) {
			free(id);

			// mmd_print_const(out, "Z");

			if (fe->export_abbr_def(a, text, len, (*t)->child->next, NULL, out, r, w, options, &first)) {
				return 1;
			} else {
				// fe->export_tokens((*t)->child->next, text, len, out, r, w, options);

//				if (first) {
//					mmd_print_const(out, "</abbr>)");
//				} else {
//					mmd_print_const(out, "</abbr>");
//				}

				// Skip over closer and id brackets
				(*t) = (*t)->next;
				return 0;
			}
		} else {
			// This is an inline abbreviation
			// These are *not* stored in the hash for later reuse
			mmd_node * temp = (*t)->child->next;

			while (temp && (temp->type == TOKEN_TEXT_WHITESPACE || temp->type == TOKEN_NBSP)) {
				temp = temp->next;
			}

			if (temp && temp->type == TOKEN_PAIR_PAREN && temp->child) {
				a = calloc(1, sizeof(abbr_def));

				// Replace key -- we are shorter, so no need to reallocate
				mmd_node * key_token = temp->child;

				memcpy(&id[1], &text[key_token->start], key_token->tail->start + key_token->tail->len - key_token->start);
				id[1 + key_token->tail->start + key_token->tail->len - key_token->start] = '\0';

				a->key = id;

				char * cur = (char *) &text[key_token->tail->start + key_token->tail->len + 1];

				while (char_is_whitespace(*cur)) {
					cur++;
				}

				// Extract plain text expansion
				size_t len = &text[(*t)->next->start] - cur;
				a->expansion = mmd_strndup(cur, &len);
				a->expansion_len = len;

				// Locate token(s) for expansion
				temp = temp->next->next;

				// Trim leading whitespace
				while (temp && char_is_whitespace(text[temp->start]) && temp->len) {
					temp->start++;
					temp->len--;
				}

				first = true;

				int result = fe->export_abbr_def(a, text, len, key_token, temp, out, r, w, options, &first);

				if (result) {
				} else {
					// fe->export_raw_text(&a->key[1], strlen(a->key) - 1, out);

					// if (first) {
					// 	mmd_print_const(out, "</abbr>)");
					// } else {
					// 	mmd_print_const(out, "</abbr>");
					// }

					(*t) = (*t)->next;
				}

				abbr_def_free(a);
				return result;
			} else {
				// Not a valid abbreviation
				free(id);
				return 1;
			}
		}

	}

	return 1;
}


static int export_implicit_citation(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, bool not_cited, format_export * fe, uint32_t options) {
	// [#...] - No locator

	if ((*t)->child && (*t)->next) {
		char * id = md_id_from_text(&text[(*t)->child->start], (*t)->next->start - (*t)->child->start, true);
		endnote_def * e = read_ctx_get_cite(r, id);

		if (e) {
			free(id);

			if (not_cited) {
				if (e->index == 0) {
					stack_push(w->used_cite_stack, e);
					e->index = (int) w->used_cite_stack->size;
					e->used = true;

					e->not_cited = true;
					fe->export_endnote_def(TOKEN_PAIR_BRACKET_CITATION, e, text, len, NULL, out, r, w, options);
					e->not_cited = false;
				}

				return 0;
			} else if (fe->export_endnote_def(TOKEN_PAIR_BRACKET_CITATION, e, text, len, NULL, out, r, w, options)) {
				return 1;
			} else {
				// Skip over closer and id brackets
				(*t) = (*t)->next;
				return 0;
			}
		} else {
			// This is an inline citation
			// These are *not* stored in the hash for later reuse
			e = calloc(1, sizeof(endnote_def));
			e->key = id;
			e->is_inline = true;
			e->text = text;
			e->len = len;

			e->content_node = (*t)->child;

			(*t)->child->type = TOKEN_CITATION_MARKER;

			if (strlen(id) && id[strlen(id) - 1] == ';') {
				e->citet = true;
				id[strlen(id) - 1] = '\0';
			}

			stack_push(w->inline_endnotes_to_free, e);

			if (not_cited) {
				if (e->index == 0) {
					stack_push(w->used_cite_stack, e);
					e->index = (int) w->used_cite_stack->size;
					e->used = true;
				}

				return 0;
			} else {
				int result = fe->export_endnote_def(TOKEN_PAIR_BRACKET_CITATION, e, text, len, NULL, out, r, w, options);

				(*t) = (*t)->next;
				return result;
			}
		}
	}

	return 1;
}

static int export_split_citation(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {
	// [...][#...] locator and id are split

	mmd_node * id_node;

	if ((*t)->type == TOKEN_PAIR_BRACKET_EMPTY) {
		id_node = (*t)->next;
	} else {
		id_node = (*t)->next->next;
	}

	if (id_node->child) {
		char * id = md_id_from_text(&text[id_node->child->start], id_node->next->start - id_node->child->start, true);
		endnote_def * e = read_ctx_get_cite(r, id);

		if (e) {
			free(id);

			if (fe->export_endnote_def(TOKEN_PAIR_BRACKET_CITATION, e, text, len, (*t)->child, out, r, w, options)) {
				return 1;
			} else {
				// Skip over closer and id brackets
				(*t) = (*t)->next;
				(*t) = (*t)->next;
				(*t) = (*t)->next;
				return 0;
			}
		} else {
			// This is an inline split citation with locator
			// These are *not* stored in the hash for later reuse
			e = calloc(1, sizeof(endnote_def));
			e->key = id;
			e->is_inline = true;
			e->text = text;
			e->len = len;
			e->content_node = (*t)->next->next->child;

			if (strlen(id) && id[strlen(id) - 1] == ';') {
				e->citet = true;
				id[strlen(id) - 1] = '\0';
			}

			stack_push(w->used_cite_stack, e);
			e->index = (int) w->used_cite_stack->size;

			stack_push(w->inline_endnotes_to_free, e);

			int result = fe->export_endnote_def(TOKEN_PAIR_BRACKET_CITATION, e, text, len, (*t)->child, out, r, w, options);

			if (!result) {
				(*t) = id_node->next;
			}

			return result;
		}
	}

	return 1;
}

static int export_implicit_footnote(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {
	// [^...]

	if ((*t)->child && (*t)->next) {
		char * id = md_id_from_text(&text[(*t)->child->start], (*t)->next->start - (*t)->child->start, true);
		endnote_def * e = read_ctx_get_note(r, id);

		if (e) {
			free(id);

			if (fe->export_endnote_def(TOKEN_PAIR_BRACKET_FOOTNOTE, e, text, len, (*t)->child, out, r, w, options)) {
				return 1;
			} else {
				// Skip over closer and id brackets
				(*t) = (*t)->next;
				return 0;
			}
		} else {
			// This is an inline footnote
			// These are *not* stored in the hash for later reuse
			e = calloc(1, sizeof(endnote_def));
			e->key = id;
			e->is_inline = true;
			e->text = text;
			e->len = len;
			e->content_node = (*t)->child;

			(*t)->child->type = TOKEN_FOOTNOTE_MARKER;

			stack_push(w->inline_endnotes_to_free, e);

			int result = fe->export_endnote_def(TOKEN_PAIR_BRACKET_FOOTNOTE, e, text, len, (*t)->child, out, r, w, options);

			(*t) = (*t)->next;
			return result;
		}
	}

	return 1;
}

static int export_implicit_glossary(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {
	// [?...]

	if ((*t)->child && (*t)->next) {
		char * id = md_id_from_text(&text[(*t)->child->start], (*t)->next->start - (*t)->child->start, true);
		endnote_def * e = read_ctx_get_glos(r, id);

		if (e) {
			free(id);

			if (fe->export_endnote_def(TOKEN_PAIR_BRACKET_GLOSSARY, e, text, len, (*t)->child, out, r, w, options)) {
				return 1;
			} else {
				// Skip over closer and id brackets
				(*t) = (*t)->next;
				return 0;
			}
		} else {
			// This is possibly an inline glossary
			// These are *not* stored in the hash for later reuse
			mmd_node * temp = (*t)->child;

			while (temp && temp->type != TOKEN_PAIR_PAREN) {
				temp = temp->next;
			}

			if (temp == NULL) {
				free(id);
				return 1;
			}

			e = calloc(1, sizeof(endnote_def));
			e->key = id;
			e->is_inline = true;
			e->text = text;
			e->len = len;
			e->content_node = (*t)->child;

			if (temp && temp->type == TOKEN_PAIR_PAREN) {
				e->title_node = temp->child;
				e->content_node = temp->next->next;

				// Trim leading whitespace
				while (e->content_node && e->content_node->len && char_is_whitespace(text[e->content_node->start])) {
					e->content_node->start++;
					e->content_node->len--;
				}
			}

			(*t)->child->type = TOKEN_GLOSSARY_MARKER;

			stack_push(w->inline_endnotes_to_free, e);

			int result;

			if (temp && temp->type == TOKEN_PAIR_PAREN) {
				result = fe->export_endnote_def(TOKEN_PAIR_BRACKET_GLOSSARY, e, text, len, temp->child, out, r, w, options);
			} else {
				result = fe->export_endnote_def(TOKEN_PAIR_BRACKET_GLOSSARY, e, text, len, (*t)->child, out, r, w, options);
			}

			(*t) = (*t)->next;
			return result;
		}
	}

	return 1;
}

static int export_implicit_variable(const char * text, mmd_node ** t, text_buffer * out, read_ctx * r, format_export * fe) {
	if ((*t)->child && (*t)->next) {
		char * label = html_id_from_text(&text[(*t)->child->start], (*t)->next->start - (*t)->child->start, true);
		meta * m = read_ctx_get_meta(r, label);
		free(label);

		if (m) {
			// Metadata
			fe->export_raw_text(m->value, m->value_len, out);
			(*t) = (*t)->next;

			return 0;
		}
	}

	return 1;
}


/// Interpret meaning of a [...] based on next token and content (e.g. link, image, abbreviation, citation, footnote, glossary, variable)
int export_token_pair(const char * text, size_t len, mmd_node ** t, text_buffer * out, read_ctx * r, write_ctx * w, format_export * fe, uint32_t options) {
	unsigned char next_type = 0;
	mmd_node * next_node = (*t)->next;

	// Is this [...], [...][...], or [...](...)
	if (next_node && next_node->next) {
		next_type = next_node->next->type;

		// Allow a single line break between the [link text] and the ()
		// Because line wrapping frequently hits us here
		if (next_type == TOKEN_NL) {
			if (next_node->next->next && next_node->next->next->type == TOKEN_PAIR_PAREN) {
				next_type = TOKEN_PAIR_PAREN;
			}
		}
	}

	int result = 1;

	switch ((*t)->type) {
		case TOKEN_PAIR_BRACKET_EMPTY:
			if (next_node) {
				next_type = next_node->type;
			}

			switch (next_type) {
				case TOKEN_PAIR_PAREN:
					// Empty inline link [](...)
					return export_inline_link(text, len, t, out, r, w, fe, options);
					break;

				case TOKEN_PAIR_BRACKET:
					// Empty reference link [][...]
					return export_split_link(text, len, t, out, r, w, fe, options);
					break;

				case TOKEN_PAIR_BRACKET_CITATION:
					// Empty citation [][#...] (no locator)
					result = export_implicit_citation(text, len, &next_node, out, r, w, false, fe, options);

					if (!result) {
						(*t) = (*t)->next;
						(*t) = (*t)->next;
					}

					return result;
					break;
			}

			break;

		case TOKEN_PAIR_BRACKET_NOT_CITED:
			if (next_node) {
				next_type = next_node->type;
			}

			switch (next_type) {
				case TOKEN_PAIR_BRACKET_CITATION:
					// Not cite [Not Cited][#...]
					result = export_implicit_citation(text, len, &next_node, out, r, w, true, fe, options);

					if (!result) {
						(*t) = (*t)->next;
						(*t) = (*t)->next;
					}

					return result;
					break;
			}

			break;

		case TOKEN_PAIR_BRACKET:

			// Plain [...]

			switch (next_type) {
				case TOKEN_PAIR_PAREN:
					// [...](...) Inline link
					return export_inline_link(text, len, t, out, r, w, fe, options);
					break;

				case TOKEN_PAIR_BRACKET:
					// [...][...] Reference link
					return export_split_link(text, len, t, out, r, w, fe, options);
					break;

				case TOKEN_PAIR_BRACKET_EMPTY:
					// Empty trailing bracket pair --> skip it
					result = export_implicit_link(text, len, t, out, r, w, fe, options);

					if (!result) {
						(*t) = (*t)->next;
					}

					return result;
					break;

				case TOKEN_PAIR_BRACKET_CITATION:
					// [...][#...] Reference citation
					return export_split_citation(text, len, t, out, r, w, fe, options);
					break;

				default:
					// Ignore whatever is next and treat as Implicit link
					return export_implicit_link(text, len, t, out, r, w, fe, options);
					break;
			}

			break;

		case TOKEN_PAIR_BRACKET_IMAGE:

			// ![...]

			switch (next_type) {
				case TOKEN_PAIR_PAREN:
					// ![...](...) Inline image
					return export_inline_image(text, len, t, out, r, w, fe, options);
					break;

				case TOKEN_PAIR_BRACKET:
					// ![...][...] Reference image
					return export_split_image(text, len, t, out, r, w, fe, options);
					break;

				case TOKEN_PAIR_BRACKET_EMPTY:
					// ![...][] Skip empty bracket pair
					result = export_implicit_image(text, len, t, out, r, w, fe, options);

					if (!result) {
						(*t) = (*t)->next;
					}

					return result;
					break;

				default:
					// Ignore whatever is next and treat as Implicit image
					return export_implicit_image(text, len, t, out, r, w, fe, options);
					break;
			}

			break;

		case TOKEN_PAIR_BRACKET_ABBREVIATION:

			// [>...]

			switch (next_type) {
				case TOKEN_PAIR_BRACKET:
					// [...][...] Reference abbreviation??
					// TODO: Decide what to do here
					break;

				case TOKEN_PAIR_BRACKET_EMPTY:
					// [>...][] Skip empty bracket pair
					result = export_implicit_abbreviation(text, len, t, out, r, w, fe, options);

					if (!result) {
						(*t) = (*t)->next;
					}

					return result;
					break;

				default:
					// Ignore whatever is next and treat as Implicit abbreviation
					return export_implicit_abbreviation(text, len, t, out, r, w, fe, options);
					break;
			}

			break;

		case TOKEN_PAIR_BRACKET_CITATION:

			// [#...]

			switch (next_type) {
				case TOKEN_PAIR_BRACKET:
					// [...][...] Reference citation??
					// TODO: Decide what to do here
					break;

				case TOKEN_PAIR_BRACKET_EMPTY:
					// [#...][] Skip empty bracket pair
					result = export_implicit_citation(text, len, t, out, r, w, false, fe, options);

					if (!result) {
						(*t) = (*t)->next;
					}

					return result;
					break;

				default:
					// Ignore whatever is next and treat as Implicit citation
					return export_implicit_citation(text, len, t, out, r, w, false, fe, options);
					break;
			}

			break;

		case TOKEN_PAIR_BRACKET_FOOTNOTE:

			// [^...]

			switch (next_type) {
				case TOKEN_PAIR_BRACKET:
					// [...][...] Reference footnote??
					// TODO: Decide what to do here
					break;

				case TOKEN_PAIR_BRACKET_EMPTY:
					// [#...][] Skip empty bracket pair
					result = export_implicit_footnote(text, len, t, out, r, w, fe, options);

					if (!result) {
						(*t) = (*t)->next;
					}

					return result;
					break;

				default:
					// Ignore whatever is next and treat as Implicit footnote
					return export_implicit_footnote(text, len, t, out, r, w, fe, options);
					break;
			}

			break;

		case TOKEN_PAIR_BRACKET_GLOSSARY:

			// [?...]

			switch (next_type) {
				case TOKEN_PAIR_BRACKET:
					// [...][...] Reference glossary??
					// TODO: Decide what to do here
					break;

				case TOKEN_PAIR_BRACKET_EMPTY:
					// [#...][] Skip empty bracket pair
					result = export_implicit_glossary(text, len, t, out, r, w, fe, options);

					if (!result) {
						(*t) = (*t)->next;
					}

					return result;
					break;

				default:
					// Ignore whatever is next and treat as Implicit glossary
					return export_implicit_glossary(text, len, t, out, r, w, fe, options);
					break;
			}

			break;

		case TOKEN_PAIR_BRACKET_VARIABLE:

			// [%...]

			switch (next_type) {
				case TOKEN_PAIR_BRACKET:
					// [...][...] Reference variable??
					// TODO: Decide what to do here
					break;

				case TOKEN_PAIR_BRACKET_EMPTY:
					// [#...][] Skip empty bracket pair
					result = export_implicit_variable(text, t, out, r, fe);

					if (!result) {
						(*t) = (*t)->next;
					}

					return result;
					break;

				default:
					// Ignore whatever is next and treat as Implicit variable
					return export_implicit_variable(text, t, out, r, fe);
					break;
			}

			break;

		default:
			fprintf(stderr, "Error parsing bracket pair\n");
			break;
	}

	return 1;
}


int raw_level_for_header(mmd_node * n) {
	switch (n->type) {
		case BLOCK_H1:
		case BLOCK_SETEXT_1:
			return 1;

		case BLOCK_H2:
		case BLOCK_SETEXT_2:
			return 2;

		case BLOCK_H3:
			return 3;

		case BLOCK_H4:
			return 4;

		case BLOCK_H5:
			return 5;

		case BLOCK_H6:
			return 6;

		default:
			return 0;
	}
}


int raw_filter_matches_format(const char * pattern, int format) {
	if (!pattern) {
		return 0;
	}

	if (strncmp("*", pattern, 1) == 0) {
		return 1;
	} else if (strncmp("{=*}", pattern, 4) == 0) {
		return 1;
	} else {
		switch (format) {
			case FORMAT_HTML:
				if (strncmp("{=html}", pattern, 6) == 0) {
					return 1;
				}

				break;

			case FORMAT_ODT:
			case FORMAT_FODT:
				if (strncmp("{=odt}", pattern, 5) == 0) {
					return 1;
				}

				break;

			case FORMAT_EPUB:
				if (strncmp("{=epub}", pattern, 6) == 0) {
					return 1;
				}

				break;

			case FORMAT_BEAMER:
			case FORMAT_LTX_TALK:
			case FORMAT_LATEX:
				if (strncmp("{=latex}", pattern, 7) == 0) {
					return 1;
				}

				break;
		}
	}

	return 0;
}

