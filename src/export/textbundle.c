/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file textbundle.c

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
#include "mmd_utilities.h"
#include "mmd_scanner.h"
#include "mmd_token_scanner.h"
#include "char.h"

#include "export_core.h"
#include "assets.h"
#include "textbundle.h"
#include "html.h"
#include "zip.h"

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	#include <libgen.h>
#endif


static char * textbundle_info_json(void) {
	return my_strdup("{\n" \
					 "\t\"version\": 2,\n" \
					 "\t\"type\": \"net.daringfireball.markdown\",\n" \
					 "\t\"transient\": false,\n" \
					 "\t\"creatorIdentifier\": \"net.multimarkdown\",\n" \
					 "}"
					);
}


static void update_meta(mmd_node * b, text_buffer * source, read_ctx * r, long long * offset) {
	if (b && source && r && offset) {

	}

	meta * m = read_ctx_get_meta(r, "css");

	if (m) {
		asset * a = read_ctx_get_asset(r, m->value);

		if (a) {
			text_buffer_replace_range(source, b->start + m->value_start, m->value_len, a->uuid, 36);
			text_buffer_replace_range(source, b->start + m->value_start, 0, "assets/", 7);
			*offset += (36 + 7 - (m->value_len));
		}
	}
}


static void update_figure(mmd_node * b, text_buffer * source, read_ctx * r, long long * offset) {
	mmd_node * content = b->content;

	while (content) {
		if (content->type == TOKEN_PAIR_PAREN) {
			char * url = &source->text[b->start + *offset + content->start + 1];

			while (char_is_whitespace(*url)) {
				url++;
			}

			switch (url[0]) {
				case '"':
				case '\'':
				case '<':
					url++;
			}

			char * end = url;

			while (!char_is_whitespace(*end)) {
				end++;
			}

			switch (end[-1]) {
				case '\'':
				case '"':
				case '>':
					end--;
			}

			char * url_copy = my_strndup(url, end - url);

			asset * a = read_ctx_get_asset(r, url_copy);

			if (a && a->stored) {
				text_buffer_replace_range(source, url - source->text, end - url, a->uuid, 36);
				text_buffer_replace_range(source, url - source->text, 0, "assets/", 7);
				*offset += (36 + 7 - (end - url));
			}

			free(url_copy);
		}

		content = content->next;
	}
}


static void update_def_link(mmd_node * b, text_buffer * source, read_ctx * r, long long * offset) {
	link_def * l = scan_ref_link(&source->text[b->start + *offset], b->len);

	if (l) {
		asset * a = read_ctx_get_asset(r, l->url);

		if (a && a->stored) {
			char * url = &source->text[b->start + *offset];

			while (url[0] != ':') {
				url++;
			}

			while (strncmp(url, l->url, strlen(l->url))) {
				url++;
			}

			text_buffer_replace_range(source, url - source->text, strlen(l->url), a->uuid, 36);
			text_buffer_replace_range(source, url - source->text, 0, "assets/", 7);
			*offset += (36 + 7 - strlen(l->url));
		}

		link_def_free(l);
	}
}


static void update_asset_urls(mmd_node * b, text_buffer * source, read_ctx * r, long long * offset) {
	while (b) {
		switch (b->type) {
			case BLOCK_META:
				update_meta(b, source, r, offset);
				break;

			case BLOCK_FIGURE:
				update_figure(b, source, r, offset);
				break;

			case BLOCK_DEF_LINK:
				update_def_link(b, source, r, offset);
				break;

			default:
				if (b->child) {
					update_asset_urls(b->child, source, r, offset);
				}

				break;
		}

		b = b->next;
	}
}


void export_textbundle(mmd_node * b, text_buffer * source, text_buffer * out, read_ctx * r, uint32_t options, const char * source_path) {
	char * data;
	size_t len;


	// Store assets
	options |= MMD_OPTION_STORE_ASSETS;

	// HTML exporting does the majority of the work
	export_html(b, source->text, out, r, options, source_path);


	// Create zip archive
	mz_zip_archive zip;
	mz_bool status = zip_new_archive(&zip);


	// Create directories
	if (!mz_zip_writer_add_mem(&zip, "assets/", NULL, 0, MZ_NO_COMPRESSION)) {
		fprintf(stderr, "Error adding assets directory to zip archive.\n");
	}


	// Add assets
	char * absolute_search_path = mmd_dirname(source_path);

	if (!archive_assets_to_zip(&zip, r, "assets/", absolute_search_path, options)) {
		fprintf(stderr, "Error adding assets to zip archive\n");
	}

	free(absolute_search_path);


	// Update urls for stored assets
	long long offset = 0;

	meta * m = read_ctx_get_meta(r, "css");

	if (m) {

	}

	update_asset_urls(b, source, r, &offset);


	// Add info.json
	data = textbundle_info_json();
	len = strlen(data);
	status = mz_zip_writer_add_mem(&zip, "info.json", data, len, MZ_BEST_COMPRESSION);
	free(data);


	// Add main content
	if (!mz_zip_writer_add_mem(&zip, "text.markdown", source->text, source->len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding main content to zip archive.\n");
	}


	// Add HTML
	if (!mz_zip_writer_add_mem(&zip, "text.html", out->text, out->len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding HTML to zip archive.\n");
	}


	// Finalize zip archive and insert in out text_buffer
	free(out->text);
	out->text = NULL;
	status = mz_zip_writer_finalize_heap_archive(&zip, (void **) & (out->text), (size_t *) & (out->len));

	if (!status) {
		fprintf(stderr, "Error finalizing zip archive.\n");
		free(out->text);
		out->text = malloc(out->capacity + 1);
		out->len = 0;
	} else {
		out->capacity = out->len;
	}

	mz_zip_writer_end(&zip);
}

