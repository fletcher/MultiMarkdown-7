/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file itmz.c

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


#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libMultiMarkdown7.h"
#include "mmd_utilities.h"
#include "text_buffer.h"

#include "outline.h"
#include "yxml.h"
#include "zip.h"


#define F(i,n) for(int i= 0;i<n;i++)

#define kYXML_BUFSIZE 4096

static char * name[] = {
	[OUTLINE_OPML] = "OPML",
	[OUTLINE_ITMZ] = "ITMZ",
};


static char * level[] = {
	[OUTLINE_OPML] = "outline",
	[OUTLINE_ITMZ] = "topic",
};


static char * header[] = {
	[OUTLINE_OPML] = "text",
	[OUTLINE_ITMZ] = "text",
};


static char * content[] = {
	[OUTLINE_OPML] = "_note",
	[OUTLINE_ITMZ] = "note",
};


/// If the source text is ITMZ or OPML, convert to MultiMarkdown text and replace the buffer
int mmd_import_outline(text_buffer * source_buffer, enum outline_type type) {
	char * ch = NULL;
	text_buffer * extracted = NULL;
	int result = 0;
	int depth = 0;

	// Prepare to parse XML
	yxml_ret_t ret;
	yxml_t * x = malloc(sizeof(yxml_t) + kYXML_BUFSIZE);
	yxml_init(x, x + 1, kYXML_BUFSIZE);

	// We need a new textbuffer for the output
	text_buffer * output = text_buffer_new(0);
	text_buffer * metadata = text_buffer_new(0);

	// Temporary storage
	text_buffer * buf = text_buffer_new(0);


	if (type == OUTLINE_ITMZ) {
		// Is this a zip file?
		if (memcmp(source_buffer->text, "PK\3\4", 4)) {
			// Not a zip
			goto cleanup;
		}

		// Get mapdata.xml
		extracted = text_buffer_new(0);
		mz_bool status = zip_binary_extract_file(source_buffer->text, source_buffer->len, "mapdata.xml", extracted);

		if (!status) {
			goto cleanup;
		}

		// Does it look like XML?
		if (strncmp("<iThoughts", extracted->text, 10)) {
			goto cleanup;
		}

		ch = extracted->text;

		// Track outline levels
		depth = -1;
	} else if (type == OUTLINE_OPML) {
		// Does it look like XML?
		if (strncmp("<?xml", source_buffer->text, 5)) {
			goto cleanup;
		}

		ch = source_buffer->text;
	}

	// Remember last element type
	char * last_e = NULL;

	// Are we in metadata
	int in_meta = 0;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			fprintf(stderr, "XML error parsing as %s %d at line %d, byte %" PRIu64 "\n", name[type], ret, x->line, x->byte);
			break;
		} else {
			switch (ret) {
				case YXML_ELEMSTART:
					if (!strcmp(level[type], x->elem)) {
						depth++;
					}

					last_e = x->elem;
					break;

				case YXML_ELEMEND:
					if (!strcmp(level[type], last_e)) {
						depth--;
					}

					last_e = x->elem;
					break;

				case YXML_ATTRVAL:
					// Append to the temporary storage
					text_buffer_append_printf(buf, "%s", x->data);
					break;

				case YXML_ATTREND:
					if (!strcmp(header[type], x->attr)) {
						// Skip central topic
						if (depth == 0) {
							buf->len = 0;
							break;
						}

						// Skip >>Preamble<<
						if (!strncmp(">>Preamble<<", buf->text, 12)) {
							buf->len = 0;
							break;
						}

						// Metadata is different
						if (!strncmp(">>Metadata<<", buf->text, 12)) {
							buf->len = 0;
							in_meta = 1;
							break;
						}

						if (in_meta) {
							// Metadata
							text_buffer_append_text(metadata, buf->text, buf->len);
							text_buffer_append_c(metadata, ':');
							text_buffer_append_c(metadata, '\t');
						} else {
							// Header
							if (!text_contains_char(buf->text, buf->len, '\n')) {
								// ATX Header
								F(i, depth) {
									text_buffer_append_c(output, '#');
								}

								text_buffer_append_c(output, ' ');
								text_buffer_append_text(output, buf->text, buf->len);
								text_buffer_append_c(output, ' ');

								F(i, depth) {
									text_buffer_append_c(output, '#');
								}
							} else {
								// Setext Header
								text_buffer_append_text(output, buf->text, buf->len);

								switch (depth) {
									case 1:
										text_buffer_append_text(output, "\n========", 9);
										break;

									default:
										text_buffer_append_text(output, "\n--------", 9);
										break;
								}
							}

							text_buffer_append_c(output, '\n');
						}
					} else if (!strcmp(content[type], x->attr)) {
						if (in_meta) {
							text_buffer_append_text(metadata, buf->text, buf->len);
							text_buffer_append_c(metadata, '\n');
						} else {
							text_buffer_append_text(output, buf->text, buf->len);
						}
					}

					buf->len = 0;
					break;

				default:
					break;
			}
		}

		ch++;
	}

cleanup:

	ret = yxml_eof(x);
	free(x);

	if (extracted) {
		text_buffer_free(extracted, 1);
	}

	if (ret < 0) {
		fprintf(stderr, "XML error parsing as %s %d at EOF\n", name[type], ret);
	} else {
		source_buffer->len = 0;
		text_buffer_append_text(source_buffer, metadata->text, metadata->len);
		text_buffer_append_text(source_buffer, output->text, output->len);

		result = 1;
	}

	text_buffer_free(output, 1);
	text_buffer_free(metadata, 1);
	text_buffer_free(buf, 1);

	return result;
}
