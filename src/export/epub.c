/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file epub.c

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

#include "export_core.h"
#include "dc.h"
#include "assets.h"
#include "epub.h"
#include "html.h"
#include "zip.h"

#if (defined(_WIN32) || defined(__WIN32__))
#else
	#include <libgen.h>
#endif


static char * epub_mimetype(void) {
	return my_strdup("application/epub+zip");
}


static char * epub_container(void) {
	return my_strdup("<?xml version=\"1.0\"?>\n" \
					 "<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n" \
					 "<rootfiles>\n" \
					 "<rootfile full-path=\"OEBPS/main.opf\" media-type=\"application/oebps-package+xml\" />\n" \
					 "</rootfiles>\n" \
					 "</container>\n");
}


static char * media_type_string[] = {
	[textCSS] = "text/css",
	[imagePNG] = "image/png",
	[imageJPEG] = "image/jpeg",
};


static char * epub_package(read_ctx * r) {
	text_buffer * buffer = text_buffer_new(0);
	meta * m;

	// Package and Metadata

	text_buffer_append_printf(buffer,
							  "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\"?>\n" \
							  "<package xmlns=\"http://www.idpf.org/2007/opf\" version=\"3.0\" unique-identifier=\"pub-id\">\n" \
							  "<metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n"
							 );

	// Identifier
	m = read_ctx_get_meta(r, "uuid");

	if (m) {
		dc_write_term(buffer, DC_IDENTIFIER, m->value, "pub-id");
	} else {
		char * uuid = uuid_new();
		dc_write_term(buffer, DC_IDENTIFIER, uuid, NULL);
		free(uuid);
	}


	// Title
	m = read_ctx_get_meta(r, "title");

	if (m) {
		dc_write_term(buffer, DC_TITLE, m->value, NULL);
	} else {
		dc_write_term(buffer, DC_TITLE, "Untitled", NULL);
	}


	// Author
	m = read_ctx_get_meta(r, "author");

	if (m) {
		dc_write_term(buffer, DC_CREATOR, m->value, NULL);
	}


	// Language
	m = read_ctx_get_meta(r, "language");

	if (m) {
		dc_write_term(buffer, DC_LANGUAGE, m->value, NULL);
	} else {
		dc_write_language(buffer, r->language);
	}


	// Date
	m = read_ctx_get_meta(r, "date");

	if (m) {
		text_buffer_append_printf(buffer, "<meta property=\"dcterms:modified\">%s</meta>\n", m->value);
	} else {
		time_t t = time(NULL);
		struct tm * today = localtime(&t);

		text_buffer_append_printf(buffer, "<meta property=\"dcterms:modified\">%d-%02d-%02d</meta>\n", today->tm_year + 1900, today->tm_mon + 1, today->tm_mday);
	}


	text_buffer_append_printf(buffer, "</metadata>\n");


	// Manifest, Spine, closure
	text_buffer_append_printf(buffer,
							  "<manifest>\n" \
							  "<item id=\"nav\" href=\"nav.xhtml\" properties=\"nav\" media-type=\"application/xhtml+xml\"/>\n" \
							  "<item id=\"main\" href=\"main.xhtml\" media-type=\"application/xhtml+xml\"/>\n"
							 );

	asset * a, * a_tmp;

	HASH_ITER(hh, r->asset_hash, a, a_tmp) {
		text_buffer_append_printf(buffer, "<item id=\"%s\" href=\"assets/%s\" media-type=\"%s\"/>\n", a->uuid, a->uuid, media_type_string[a->type]);
	}


	text_buffer_append_printf(buffer,
							  "</manifest>\n" \
							  "<spine>\n" \
							  "<itemref idref=\"main\"/>\n" \
							  "</spine>\n" \
							  "</package>\n"
							 );


	char * result = buffer->text;
	text_buffer_free(buffer, 0);
	return result;
}


static void export_toc_entry(text_buffer * out, size_t * counter, int level, int min, int max, read_ctx * r, write_ctx * w, uint32_t options) {
	header * h, * next;
	int h_level, next_level;

	mmd_print_const(out, "\n<ol>\n");

	while (*counter < r->header_stack->size) {
		h = stack_peek_index(r->header_stack, *counter);
		h_level = raw_level_for_header(h->node);

		if (h_level < min || h_level > max) {
			// Ignore this header
		} else {
			if (h_level >= level) {
				// This header is a direct descendant of the parent
				text_buffer_append_printf(out, "<li><a href=\"main.xhtml#%s\">", h->key);
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

	mmd_print_const(out, "</ol>\n");
}


static char * epub_nav(read_ctx * r, write_ctx * w, uint32_t options) {
	text_buffer * buffer = text_buffer_new(0);
	meta * m;

	text_buffer_append_printf(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE html>\n<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:epub=\"http://www.idpf.org/2007/ops\">\n");

	// Title
	m = read_ctx_get_meta(r, "title");

	if (m) {
		text_buffer_append_printf(buffer, "<head>\n<title>%s</title>\n</head>\n", m->value);
	} else {
		text_buffer_append_printf(buffer, "<head>\n<title>Untitled</title>\n</head>\n");
	}


	// TOC
	text_buffer_append_printf(buffer,
							  "<body>\n<nav epub:type=\"toc\">\n" \
							  "<h2>Table of Contents</h2>\n"
							 );


	size_t counter = 0;
	export_toc_entry(buffer, &counter, 0, 0, 6, r, w, options);

	text_buffer_append_printf(buffer, "</nav>\n</body>\n</html>\n");

	char * result = buffer->text;
	text_buffer_free(buffer, 0);
	return result;
}


void export_epub(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, uint32_t options, const char * source_path) {
	char * data;
	size_t len;


	// Force complete document
	char old_complete = r->write_complete;
	r->write_complete = 1;

	// Store assets
	options |= MMD_OPTION_STORE_ASSETS;

	// HTML exporting does the majority of the work
	export_html(b, text, out, r, options, source_path);

	// Insert xml declaration header
	data = my_strdup("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	text_buffer_prepend_text(out, data, strlen(data));
	free(data);


	// Restore prior state
	r->write_complete = old_complete;


	// Create zip archive
	mz_zip_archive zip;
	mz_bool status = zip_new_archive(&zip);


	// Add mimetype
	data = epub_mimetype();
	len = strlen(data);

	if (!mz_zip_writer_add_mem(&zip, "mimetype", data, len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding mimetype to zip archive.\n");
	}

	free(data);


	// Create directories
	if (!mz_zip_writer_add_mem(&zip, "OEBPS/", NULL, 0, MZ_NO_COMPRESSION)) {
		fprintf(stderr, "Error adding OEBPS directory to zip archive.\n");
	}

	if (!mz_zip_writer_add_mem(&zip, "META-INF/", NULL, 0, MZ_NO_COMPRESSION)) {
		fprintf(stderr, "Error adding META-INF directory to zip archive.\n");
	}


	// Add container
	data = epub_container();
	len = strlen(data);

	if (!mz_zip_writer_add_mem(&zip, "META-INF/container.xml", data, len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding container.xml to zip archive.\n");
	}

	free(data);


	// Add package
	data = epub_package(r);
	len = strlen(data);

	if (!mz_zip_writer_add_mem(&zip, "OEBPS/main.opf", data, len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding main.opf to zip archive.\n");
	}

	free(data);


	// Add navigation
	write_ctx * w = write_ctx_new();
	data = epub_nav(r, w, options);
	len = strlen(data);

	if (!mz_zip_writer_add_mem(&zip, "OEBPS/nav.xhtml", data, len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding nav.xhtml to zip archive.\n");
	}

	free(data);
	write_ctx_free(w);


	// Add main content
	if (!mz_zip_writer_add_mem(&zip, "OEBPS/main.xhtml", out->text, out->len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding main content to zip archive.\n");
	}


	// Add assets
	char * absolute_search_path = mmd_dirname(source_path);

	if (!archive_assets_to_zip(&zip, r, "OEBPS/assets/", absolute_search_path, options)) {
		fprintf(stderr, "Error adding assets to zip archive\n");
	}

	free(absolute_search_path);


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
