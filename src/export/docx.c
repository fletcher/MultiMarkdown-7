/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file docx.c

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
#include "docx.h"
#include "html.h"
#include "zip.h"

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	#include <libgen.h>
#endif


#define F(i,n) for(int i= 0;i<n;i++)


static void export_docx_tokens(mmd_node * t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options);
static void export_docx_blocks(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options);


typedef struct {
	char			padding;

	int 			check_run;

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
} docx_parse_rule;

static docx_parse_rule rules[256] = {
	[BLOCK_PARA]					= { 2, 0, "<w:p>", 0, DESCEND_CONTENT, 0, "</w:p>", 0, 0, 0, 0 },

	[TOKEN_TEXT]					= { 2, 1, "<w:t>", 0, 0, 0, "</w:t>", 0, 0, 0, 0 },
	[TOKEN_TEXT_WHITESPACE]			= { 0, 1, "<w:t> ", 0, NONE, 0, "</w:t>", 0, 0, 0, 0 },
};


static smart_quote headers_compat[6] = {
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading1\"/></w:pPr>", "</w:p>", 0, 0  },
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading2\"/></w:pPr>", "</w:p>", 0, 0  },
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading3\"/></w:pPr>", "</w:p>", 0, 0  },
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading4\"/></w:pPr>", "</w:p>", 0, 0  },
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading5\"/></w:pPr>", "</w:p>", 0, 0  },
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading6\"/></w:pPr>", "</w:p>", 0, 0  },
};

static smart_quote headers[6] = {
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading1\"/></w:pPr>", "</w:p>", 0, 0  },
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading2\"/></w:pPr>", "</w:p>", 0, 0  },
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading3\"/></w:pPr>", "</w:p>", 0, 0  },
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading4\"/></w:pPr>", "</w:p>", 0, 0  },
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading5\"/></w:pPr>", "</w:p>", 0, 0  },
	{ "<w:p><w:pPr><w:pStyle w:val=\"Heading6\"/></w:pPr>", "</w:p>", 0, 0  },
};


/// Open a new <w:r> if needed
static void check_run(text_buffer * out, write_ctx * w) {
	if (w->in_run) {
		return;
	}

	mmd_print_const(out, "<w:r>");
	w->in_run = true;
}


/// Close <w:r> if one is open
static void close_run(text_buffer * out, write_ctx * w) {
	if (w->in_run) {
		mmd_print_const(out, "</w:r>");
		w->in_run = false;
	}
}


static void export_docx_token(mmd_node ** t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	docx_parse_rule rule = rules[(*t)->type];

	if (rule.check_run) {
		check_run(out, w);
	}

	switch ((*t)->type) {
		// Ignore these completely
		case TOKEN_ATX_MARKER:
			break;

		default:
			if (rule.prefix) {
				text_buffer_append_text(out, rule.prefix, rule.prefix_len);
				w->padding = rule.pre_descent_padding;
			}

			if (rule.descent & DESCEND_CHILD) {
				if ((*t)->child) {
					export_docx_tokens((*t)->child, text, len, out, r, w, options);
				}
			}

			if (rule.descent & DESCEND_CONTENT) {
			}

			if (rule.descent & DESCEND_RAW) {
				if ((*t)->child) {
					// export_html_raw_text(&text[(*t)->child->start], (*t)->child->tail->start + (*t)->child->tail->len - (*t)->child->start, out);
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

	if (!rule.check_run) {
		close_run(out, w);
	}
}


static void export_docx_tokens(mmd_node * t, const char * text, size_t len, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	while (t) {
		export_docx_token(&t, text, len, out, r, w, options);

		t = t->next;
	}
}


static void export_docx_block(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	docx_parse_rule rule = rules[b->type];

	switch (b->type) {

		case BLOCK_H1:
		case BLOCK_H2:
		case BLOCK_H3:
		case BLOCK_H4:
		case BLOCK_H5:
		case BLOCK_H6: {
			pad(out, 2, w);

			int level = b->type - BLOCK_H1 + read_ctx_get_header_level(r, FORMAT_DOCX);

			if (options & MMD_OPTION_COMPATIBILITY) {
				if ((level >= 0) && (level < 6)) {
					text_buffer_append_text(out, headers_compat[level].opener, headers_compat[level].opener_len);
				}
			} else {
				// TODO: Fix this
				text_buffer_append_text(out, headers[level].opener, headers[level].opener_len);
			}

			export_docx_tokens(b->content, &text[b->start], b->len, out, r, w, options);

			text_buffer_trim_trailing_whitespace(out);

			if ((level >= 0) && (level < 6)) {
				text_buffer_append_text(out, headers[level].closer, headers[level].closer_len);
			}

			w->padding = 0;
		}
		break;

		default:
			pad(out, rule.padding, w);

			if (rule.prefix) {
				text_buffer_append_text(out, rule.prefix, rule.prefix_len);
				w->padding = rule.pre_descent_padding;
			}

			if (rule.descent & DESCEND_CHILD) {
				export_docx_blocks(b->child, &text[b->start], out, r, w, options);
			}

			if (rule.descent & DESCEND_CONTENT) {
				export_docx_tokens(b->content, &text[b->start], b->len, out, r, w, options);
			}

			if (rule.descent & DESCEND_LINES) {
				if (w->in_recursive) {
					// export_html_lines_content(b->child, &text[b->start], out);
				} else {
					// export_html_lines(b->child, &text[b->start], out);
				}
			}

			if (rule.descent & DESCEND_LINES_RAW) {
				// export_html_lines_raw_content((mmd_line_node *)b->child, &text[b->start], out);
			}

			if (rule.descent & DESCEND_RAW) {

			}

			pad(out, rule.pad_post_descent, w);

			if (rule.suffix) {
				text_buffer_append_text(out, rule.suffix, rule.suffix_len);
			}

			w->padding = rule.post_suffix_padding;
			break;
	}
}


static void export_docx_blocks(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, write_ctx * w, uint32_t options) {
	while (b) {
		if (w->skip_blocks) {
			w->skip_blocks--;
		} else {
			export_docx_block(b, text, out, r, w, options);
		}

		b = b->next;
	}
}


static char * relationships(void) {
	return my_strdup("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n" \
					 "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n" \
					 "  <Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>\n" \
					 "</Relationships>\n"
					);
}


static char * content_types(void) {
	return my_strdup("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n" \
					 "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n" \
					 "  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n" \
					 "  <Default Extension=\"xml\" ContentType=\"application/xml\"/>\n" \
					 "  <Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>\n" \
					 "  <Override PartName=\"/word/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>\n" \
					 "</Types>\n"
					);
}


static char * document_rels(void) {
	return my_strdup("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
					 "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">" \
					 "    <Relationship Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Id=\"rId7\" Target=\"styles.xml\" />\n" \
					 "</Relationships>"
					);
}


static char * styles(void) {
	return my_strdup("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n" \
					 "<w:styles xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">\n" \
					 "	<w:style w:type=\"paragraph\" w:styleId=\"Normal\" w:default=\"1\">\n" \
					 "		<w:name w:val=\"Normal\"/>\n" \
					 "		<w:qFormat/>\n" \
					 "	</w:style>\n" \
					 "	<w:style w:type=\"paragraph\" w:styleId=\"Heading1\">\n" \
					 "		<w:name w:val=\"Heading 1\"/>\n" \
					 "		<w:basedOn w:val=\"Normal\"/>\n" \
					 "		<w:qFormat/>\n" \
					 "		<w:pPr>\n" \
					 "		<w:spacing w:before=\"240\" w:after=\"120\"/>\n" \
					 "		</w:pPr>\n" \
					 "		<w:rPr>\n" \
					 "		<w:sz w:val=\"36\"/>\n" \
					 "		</w:rPr>\n" \
					 "	</w:style>\n" \
					 "	<w:style w:type=\"paragraph\" w:styleId=\"Heading2\">\n" \
					 "		<w:name w:val=\"Heading2 \"/>\n" \
					 "		<w:basedOn w:val=\"Normal\"/>\n" \
					 "		<w:qFormat/>\n" \
					 "		<w:pPr>\n" \
					 "		<w:spacing w:before=\"240\" w:after=\"120\"/>\n" \
					 "		</w:pPr>\n" \
					 "		<w:rPr>\n" \
					 "		<w:sz w:val=\"36\"/>\n" \
					 "		</w:rPr>\n" \
					 "	</w:style>\n" \
					 "</w:styles>\n"
					);
}


static void export_docx_header(text_buffer * out) {
	mmd_print_const(out, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");

	mmd_print_const(out, "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">\n  <w:body>\n");
}


static void export_docx_footer(text_buffer * out) {
	mmd_print_const(out, "  </w:body>\n</w:document>\n");
}




/// Precalculate lengths of all constant strings
void precalculate_docx_rules(docx_parse_rule * rules, int n) {
	F(i, n) {
		if (rules[i].prefix) {
			rules[i].prefix_len = (int) strlen(rules[i].prefix);
		}

		if (rules[i].suffix) {
			rules[i].suffix_len = (int) strlen(rules[i].suffix);
		}
	}
}


void export_docx(mmd_node * b, const char * text, text_buffer * out, read_ctx * r, uint32_t options, const char * source_path) {
	precalculate_docx_rules(rules, sizeof(rules) / sizeof(rules[0]));

	precalculate_quotes(headers, sizeof(headers) / sizeof(headers[0]));
	precalculate_quotes(headers_compat, sizeof(headers_compat) / sizeof(headers_compat[0]));

	char * data;
	size_t len;

	if (source_path) {

	}

	// Process AST and create base file
	write_ctx * w = write_ctx_new();

	export_docx_header(out);

	export_docx_blocks(b, text, out, r, w, options);

	// export_docx_endnotes()

	export_docx_footer(out);

	pad(out, 1, w);
	write_ctx_free(w);

	if (MMD_OUT_FORMAT_FROM_OPTS(options) == FORMAT_DOCX_FLAT) {
		return;
	}


	// Create zip archive
	mz_zip_archive * zip = malloc(sizeof(mz_zip_archive));
	mz_bool status = zip_new_archive(zip);


	// Create directories
	if (!mz_zip_writer_add_mem(zip, "_rels/", NULL, 0, MZ_NO_COMPRESSION)) {
		fprintf(stderr, "Error adding _rels directory to zip archive.\n");
	}

	if (!mz_zip_writer_add_mem(zip, "word/", NULL, 0, MZ_NO_COMPRESSION)) {
		fprintf(stderr, "Error adding _rels directory to zip archive.\n");
	}

	if (!mz_zip_writer_add_mem(zip, "word/_rels", NULL, 0, MZ_NO_COMPRESSION)) {
		fprintf(stderr, "Error adding _rels directory to zip archive.\n");
	}


	// Create relationships
	data = relationships();
	len = strlen(data);

	if (!mz_zip_writer_add_mem(zip, "_rels/.rels", data, len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding relationships to zip archive.\n");
	}

	free(data);


	// Create content types
	data = content_types();
	len = strlen(data);

	if (!mz_zip_writer_add_mem(zip, "[Content_Types].xml", data, len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding content types to zip archive.\n");
	}

	free(data);


	// Add main content
	if (!mz_zip_writer_add_mem(zip, "word/document.xml", out->text, out->len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding main content to zip archive.\n");
	}


	// Add document.xml.rels
	data = document_rels();
	len = strlen(data);

	if (!mz_zip_writer_add_mem(zip, "word/_rels/document.xml.rels", data, len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding styles.xml content to zip archive.\n");
	}

	free(data);


	// Add styles.xml
	data = styles();
	len = strlen(data);

	if (!mz_zip_writer_add_mem(zip, "word/styles.xml", data, len, MZ_BEST_COMPRESSION)) {
		fprintf(stderr, "Error adding styles.xml content to zip archive.\n");
	}

	free(data);


	// Finalize zip archive and insert in out text_buffer
	free(out->text);
	out->text = NULL;
	status = mz_zip_writer_finalize_heap_archive(zip, (void **) & (out->text), (size_t *) & (out->len));

	if (!status) {
		fprintf(stderr, "Error finalizing zip archive.\n");
		free(out->text);
		out->text = malloc(out->capacity + 1);
		out->len = 0;
	} else {
		out->capacity = out->len;
	}

	mz_zip_writer_end(zip);
	free(zip);
}
