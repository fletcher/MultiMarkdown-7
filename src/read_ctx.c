/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file read_ctx.c

	@brief


	@author	Fletcher T. Penney
	@bug

**/

/*

	MIT License

	Copyright (c) 2024-2025 Fletcher T. Penney

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

#include "libMultiMarkdown.h"
#include "read_ctx.h"
#include "mmd_node.h"
#include "mmd_span_parser.h"
#include "stack.h"

/// strndup not available on all platforms
static char * my_strndup(const char * source, size_t n) {
	if (source == NULL) {
		return NULL;
	}

	size_t len = 0;
	char * result;
	const char * test = source;

	// strlen is too slow if strlen(source) >> n
	for (len = 0; len < n; ++len) {
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

	return result;
}


read_ctx * read_ctx_new(uint32_t options) {
	read_ctx * c = calloc(1, sizeof(read_ctx));

	if (c) {
		c->allow_meta = !(options & MMD_OPTION_COMPATIBILITY);

		c->token_pair_stack = stack_new(32);

		c->header_stack = stack_new(16);
		c->random_header_seed = (uint16_t) rand();

		c->language = MMD_LANGUAGE_FROM_OPTS(options);
		c->quotes_language = MMD_SMART_QUOTE_FROM_OPTS(options);
	}

	return c;
}


void header_free(header * h) {
	if (h) {
		free(h->key);
		free(h);
	}
}


void meta_free(meta * m) {
	if (m) {
		free(m->key);
		free(m->value);
		free(m);
	}
}


void link_def_free(link_def * l) {
	if (l) {
		free(l->key);
		free(l->url);
		free(l->title);

		attr * a = l->attributes;
		attr * next;

		while (a) {
			next = a->next;

			free(a->key);
			free(a->value);
			free(a);

			a = next;
		}

		free(l);
	}
}


void abbr_def_free(abbr_def * a) {
	if (a) {
		free(a->key);
		free(a->expansion);
		free(a);
	}
}


void endnote_def_free(endnote_def * e) {
	if (e) {
		free(e->key);
		free(e);
	}
}


void read_ctx_reset(read_ctx * c, uint32_t options) {
	if (c) {
		c->allow_meta = !(options & MMD_OPTION_COMPATIBILITY);

		c->token_pair_stack->size = 0;

		while (c->header_stack->size) {
			header_free(stack_pop(c->header_stack));
		}

		meta * m, * m_tmp;

		HASH_ITER(hh, c->meta_hash, m, m_tmp) {
			HASH_DEL(c->meta_hash, m);
			meta_free(m);
		}

		link_def * l, * l_tmp;

		HASH_ITER(hh, c->link_def_hash, l, l_tmp) {
			HASH_DEL(c->link_def_hash, l);
			link_def_free(l);
		}

		abbr_def * a, * a_tmp;

		HASH_ITER(hh, c->abbr_def_hash, a, a_tmp) {
			HASH_DEL(c->abbr_def_hash, a);
			abbr_def_free(a);
		}

		endnote_def * e, * e_tmp;

		HASH_ITER(hh, c->cite_def_hash, e, e_tmp) {
			HASH_DEL(c->cite_def_hash, e);
			endnote_def_free(e);
		}

		HASH_ITER(hh, c->glos_def_hash, e, e_tmp) {
			HASH_DEL(c->glos_def_hash, e);
			endnote_def_free(e);
		}

		HASH_ITER(hh, c->note_def_hash, e, e_tmp) {
			HASH_DEL(c->note_def_hash, e);
			endnote_def_free(e);
		}
	}
}

void read_ctx_free(read_ctx * c) {
	if (c) {
		read_ctx_reset(c, 0);

		while (c->header_stack->size) {
			header_free(stack_pop(c->header_stack));
		}

		stack_free(c->header_stack);

		stack_free(c->token_pair_stack);

		free(c);
	}
}


void read_ctx_store_abbr(read_ctx * c, abbr_def * a) {
	if (c && a) {
		abbr_def * temp;

		// Store in hash using key
		if (a->key && a->key[0] != '\0') {
			HASH_FIND_STR(c->abbr_def_hash, a->key, temp);

			// Don't replace existing abbr with same key
			if (!temp) {
				HASH_ADD_KEYPTR(hh, c->abbr_def_hash, a->key, strlen(a->key), a);
			} else {
				abbr_def_free(a);
			}
		} else {
			abbr_def_free(a);
		}
	}
}


void read_ctx_store_link(read_ctx * c, link_def * l) {
	if (c && l) {
		link_def * temp;

		// Store in hash using key
		if (l->key && l->key[0] != '\0') {
			HASH_FIND_STR(c->link_def_hash, l->key, temp);

			// Don't replace existing link with same key
			if (!temp) {
				HASH_ADD_KEYPTR(hh, c->link_def_hash, l->key, strlen(l->key), l);
			} else {
				link_def_free(l);
			}
		} else {
			link_def_free(l);
		}
	}
}


void read_ctx_store_internal_link(read_ctx * c, const char * text, size_t len, bool require_odd_count) {
	if (c && text && len) {
		link_def * l = calloc(1, sizeof(link_def));

		l->key = html_id_from_text(text, len, require_odd_count);

		l->url = malloc(sizeof(char) * (strlen(l->key) + 2));
		l->url[0] = '#';
		memcpy(&l->url[1], l->key, strlen(l->key) + 1);
		l->url_len = strlen(l->url);

		read_ctx_store_link(c, l);
	}
}


void read_ctx_store_internal_link_key(read_ctx * c, const char * key, size_t key_len) {
	if (c && key && key_len) {
		// Only if it doesn't exist already
		link_def * temp;
		HASH_FIND_STR(c->link_def_hash, key, temp);

		if (!temp) {
			link_def * l = calloc(1, sizeof(link_def));

			l->key = my_strndup(key, key_len);
			l->url = malloc(sizeof(char) * (key_len + 2));
			l->url[0] = '#';
			memcpy(&l->url[1], l->key, key_len + 1);
			l->url_len = key_len + 1;

			read_ctx_store_link(c, l);
		}
	}
}


void read_ctx_store_header(read_ctx * c, const char * text, size_t len, mmd_node * n, const char * key, size_t key_len) {
	if (c && text && n && key && len && key_len) {
		header * h = calloc(1, sizeof(header));

		h->key = my_strndup(key, key_len);
		h->node = n;
		h->text = text;
		h->text_len = len;

		stack_push(c->header_stack, h);
	}
}


abbr_def * read_ctx_get_abbr(read_ctx * c, char * key) {
	abbr_def * a = NULL;

	if (c && key && c->abbr_def_hash) {
		HASH_FIND_STR(c->abbr_def_hash, key, a);
	}

	return a;
}


link_def * read_ctx_get_link(read_ctx * c, char * key) {
	link_def * l = NULL;

	if (c && key && c->link_def_hash) {
		HASH_FIND_STR(c->link_def_hash, key, l);
	}

	if (!l) {
		// If `FOO BAR` didn't work, try `foobar` as a backup
		char * id = html_id_from_text(key, strlen(key), true);

		if (c && id && c->link_def_hash) {
			HASH_FIND_STR(c->link_def_hash, id, l);
		}

		free(id);
	}

	return l;
}


void read_ctx_store_meta(read_ctx * c, meta * m) {
	if (c && m) {
		meta * temp;

		// Store in hash using key
		if (m->key && m->key[0] != '\0') {
			HASH_FIND_STR(c->meta_hash, m->key, temp);

			// Don't replace existing meta with same key
			if (!temp) {
				HASH_ADD_KEYPTR(hh, c->meta_hash, m->key, strlen(m->key), m);

				// Handle special metadata instances
				if (strcmp(m->key, "baseheaderlevel") == 0) {
					c->base_header_level = atoi(m->value);
				} else if (strcmp(m->key, "epubheaderlevel") == 0) {
					c->epub_header_level = atoi(m->value);
				} else if (strcmp(m->key, "htmlheaderlevel") == 0) {
					c->html_header_level = atoi(m->value);
				} else if (strcmp(m->key, "xhtmlheaderlevel") == 0) {
					c->xhtml_header_level = atoi(m->value);
				} else if (strcmp(m->key, "latexheaderlevel") == 0) {
					c->latex_header_level = atoi(m->value);
				} else if (strcmp(m->key, "odfheaderlevel") == 0) {
					c->odf_header_level = atoi(m->value);
				} else if (strcmp(m->key, "language") == 0) {
					if (strncmp(m->value, "de", 2) == 0) {
						c->language = LANGUAGE_DE;

						if (c->quotes_language != QUOTES_GERMAN_GUILLEMETS) {
							c->quotes_language = QUOTES_GERMAN;
						}
					} else if (strncmp(m->value, "es", 2) == 0) {
						c->language = LANGUAGE_ES;
						c->quotes_language = QUOTES_SPANISH;
					} else if (strncmp(m->value, "fr", 2) == 0) {
						c->language = LANGUAGE_FR;
						c->quotes_language = QUOTES_FRENCH;
					} else if (strncmp(m->value, "he", 2) == 0) {
						c->language = LANGUAGE_HE;
						c->quotes_language = QUOTES_ENGLISH;
					} else if (strncmp(m->value, "nl", 2) == 0) {
						c->language = LANGUAGE_NL;
						c->quotes_language = QUOTES_DUTCH;
					} else if (strncmp(m->value, "sv", 2) == 0) {
						c->language = LANGUAGE_SV;
						c->quotes_language = QUOTES_SWEDISH;
					} else {
						c->language = LANGUAGE_EN;
						c->quotes_language = QUOTES_ENGLISH;
					}
				} else if (strcmp(m->key, "quoteslanguage") == 0) {
					if ((strncmp(m->value, "dutch", 5) == 0) ||
							(strncmp(m->value, "nl", 2) == 0)) {
						c->quotes_language = QUOTES_DUTCH;
					} else if ((strncmp(m->value, "french", 6) == 0) ||
							   (strncmp(m->value, "fr", 2) == 0)) {
						c->quotes_language = QUOTES_FRENCH;
					} else if (strncmp(m->value, "germanguillemets", 16) == 0) {
						c->quotes_language = QUOTES_GERMAN_GUILLEMETS;
					} else if ((strncmp(m->value, "german", 6) == 0) ||
							   (strncmp(m->value, "de", 2) == 0)) {
						c->quotes_language = QUOTES_GERMAN;
					} else if ((strncmp(m->value, "spanish", 7) == 0) ||
							   (strncmp(m->value, "es", 2) == 0)) {
						c->quotes_language = QUOTES_DUTCH;
					} else if ((strncmp(m->value, "swedish", 7) == 0) ||
							   (strncmp(m->value, "sv", 2) == 0)) {
						c->quotes_language = QUOTES_DUTCH;
					} else {
						c->quotes_language = QUOTES_ENGLISH;
					}
				}
			} else {
				meta_free(m);
			}
		} else {
			meta_free(m);
		}
	}
}


meta * read_ctx_get_meta(read_ctx * c, char * key) {
	meta * m = NULL;

	if (c && key && c->meta_hash) {
		HASH_FIND_STR(c->meta_hash, key, m);
	}

	return m;
}


int read_ctx_store_cite(read_ctx * c, endnote_def * e) {
	if (c && e) {
		endnote_def * temp;

		if (e->key && e->key[0] != '\0') {
			HASH_FIND_STR(c->cite_def_hash, e->key, temp);

			if (!temp) {
				HASH_ADD_KEYPTR(hh, c->cite_def_hash, e->key, strlen(e->key), e);
				return 0;
			} else {
				endnote_def_free(e);
			}
		} else {
			endnote_def_free(e);
		}
	}

	return 1;
}


int read_ctx_store_glos(read_ctx * c, endnote_def * e) {
	if (c && e) {
		endnote_def * temp;

		if (e->key && e->key[0] != '\0') {
			HASH_FIND_STR(c->glos_def_hash, e->key, temp);

			if (!temp) {
				HASH_ADD_KEYPTR(hh, c->glos_def_hash, e->key, strlen(e->key), e);

				return 0;
			} else {
				endnote_def_free(e);
			}
		} else {
			endnote_def_free(e);
		}
	}

	return 1;
}


int read_ctx_store_note(read_ctx * c, endnote_def * e) {
	if (c && e) {
		endnote_def * temp;

		if (e->key && e->key[0] != '\0') {
			HASH_FIND_STR(c->note_def_hash, e->key, temp);

			if (!temp) {
				HASH_ADD_KEYPTR(hh, c->note_def_hash, e->key, strlen(e->key), e);
				return 0;
			} else {
				endnote_def_free(e);
			}
		} else {
			endnote_def_free(e);
		}
	}

	return 1;
}


endnote_def * read_ctx_get_cite(read_ctx * c, char * key) {
	endnote_def * e = NULL;

	if (c && key && c->cite_def_hash) {
		HASH_FIND_STR(c->cite_def_hash, key, e);

		if (e) {
			e->citet = false;
		}
	}

	if (!e && strlen(key) > 1) {
		// If `#foo;` didn't work, try `#foo` as a backup (e.g. citet vs citep)
		char * id = my_strndup(key, strlen(key));

		if (id[strlen(id) - 1] == ';') {
			id[strlen(id) - 1] = '\0';

			if (c && id && c->cite_def_hash) {
				HASH_FIND_STR(c->cite_def_hash, id, e);
			}

			if (e) {
				e->citet = true;
			}
		}

		free(id);
	}

	return e;
}


endnote_def * read_ctx_get_glos(read_ctx * c, char * key) {
	endnote_def * e = NULL;

	if (c && key && c->glos_def_hash) {
		HASH_FIND_STR(c->glos_def_hash, key, e);
	}

	return e;
}


endnote_def * read_ctx_get_note(read_ctx * c, char * key) {
	endnote_def * e = NULL;

	if (c && key && c->note_def_hash) {
		HASH_FIND_STR(c->note_def_hash, key, e);
	}

	return e;
}


int read_ctx_get_header_level(read_ctx * c, int format) {
	int r = 0;

	if (c->base_header_level) {
		r = c->base_header_level - 1;
	}

	switch (format) {
		case FORMAT_HTML:
			if (c->html_header_level) {
				r = c->html_header_level - 1;
			}

			break;

		case FORMAT_LATEX:
			if (c->latex_header_level) {
				r = c->latex_header_level - 1;
			}

			break;
	}

	return r;
}
