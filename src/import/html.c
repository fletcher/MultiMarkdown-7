/**

	libMultiMarkdown7 -- C parser for Markdown with additional features and multiple output formats.

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


#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libMultiMarkdown7.h"
#include "text_buffer.h"

#include "char.h"
#include "html.h"
#include "yxml.h"

#define F(i,n) for(int i= 0;i<n;i++)

#define kYXML_BUFSIZE 4096

typedef yxml_ret_t (*xml_parse_func)(text_buffer *, char **, yxml_t *);

typedef struct {
	const char * element;
	const char * prefix;
	const char * suffix;
	xml_parse_func f;
} html_element;


static yxml_ret_t xml_parse_elem(text_buffer * out, char ** source, yxml_t * x);


static void append_content(text_buffer * out, yxml_t * x) {
	if (char_is_lead_multibyte(x->data[0])) {
		if (!strcmp(x->data, "“")) {
			text_buffer_append_c(out, '"');
		} else if (!strcmp(x->data, "”")) {
			text_buffer_append_c(out, '"');
		} else if (!strcmp(x->data, "’")) {
			text_buffer_append_c(out, '\'');
		} else if (!strcmp(x->data, "‘")) {
			text_buffer_append_c(out, '\'');
		} else if (!strcmp(x->data, "–")) {
			text_buffer_append_text(out, "--", 2);
		} else if (!strcmp(x->data, "—")) {
			text_buffer_append_text(out, "---", 3);
		} else if (!strcmp(x->data, "…")) {
			text_buffer_append_text(out, "...", 3);
		} else {
			text_buffer_append_printf(out, "%s", x->data);
		}
	} else {
		switch (x->data[0]) {
			case '[':
			case ']':
			case '\\':
			case '*':
			case '_':
				text_buffer_append_c(out, '\\');
				text_buffer_append_c(out, x->data[0]);
				break;

			default:
				text_buffer_append_printf(out, "%s", x->data);
				break;
		}
	}
}


static yxml_ret_t parse_meta(text_buffer * out, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;

	text_buffer * buf = text_buffer_new(0);

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			goto exit;
		}

		switch (ret) {
			case YXML_ATTRSTART:
				buf->len = 0;
				break;

			case YXML_ATTRVAL:
				text_buffer_append_printf(buf, "%s", x->data);
				break;

			case YXML_ATTREND:
				if (!strcmp(x->attr, "name")) {
					text_buffer_append_text(out, buf->text, buf->len);
					text_buffer_append_text(out, ":\t", 2);
				} else if (!strcmp(x->attr, "content")) {
					text_buffer_append_text(out, buf->text, buf->len);
					text_buffer_append_text(out, "  \n", 3);
				}

				break;

			case YXML_ELEMEND:
				goto leave;
				break;

			default:
				break;
		}

		ch++;
	}

leave:

exit:
	text_buffer_free(buf, 1);
	*source = ch;
	return ret;
}


static yxml_ret_t parse_ignore(text_buffer * out, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMEND:
				goto leave;
				break;

			case YXML_ELEMSTART:
				ch++;
				ret = parse_ignore(out, &ch, x);

				if (ret < 0) {
					goto exit;
				}

				break;

			default:
				break;
		}

		ch++;
	}

leave:

exit:
	*source = ch;
	return ret;
}


static yxml_ret_t parse_div(text_buffer * out, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;

	text_buffer * buf = text_buffer_new(0);

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				ch++;
				ret = xml_parse_elem(out, &ch, x);

				if (ret < 0) {
					goto exit;
				}

				break;

			case YXML_ATTRSTART:
				buf->len = 0;
				break;

			case YXML_ATTRVAL:
				text_buffer_append_printf(buf, "%s", x->data);
				break;

			case YXML_ATTREND:
				if (!strcmp(x->attr, "class")) {
					if (!strcmp(buf->text, "TOC")) {
						ch++;
						// Ignore everything inside this <div>
						ret = parse_ignore(out, &ch, x);
						text_buffer_append_text(out, "{{TOC}}\n\n", 9);
						goto exit;
					}
				}

				break;

			case YXML_CONTENT:

				// May be one or several characters
				if (strcmp(x->elem, "html") && strcmp(x->elem, "head") && strcmp(x->elem, "body") && strcmp(x->elem, "head")) {
					// Ignore extra stuff and whitespace, at least for now
					append_content(out, x);
				}

				break;

			case YXML_ELEMEND:
				goto leave;
				break;

			default:
				break;
		}

		ch++;
	}

leave:

exit:
	text_buffer_free(buf, 1);
	*source = ch;
	return ret;
}


static html_element elements[] = {
	{ "html", NULL, NULL, NULL },
	{ "head", NULL, "\n", NULL },
	{ "title", "title:\t", "  \n", NULL },
	{ "meta", NULL, NULL, &parse_meta },
	{ "div", NULL, NULL, &parse_div },
	{ "h1", "# ", " #\n\n", NULL },
	{ "h2", "## ", " ##\n\n", NULL },
	{ "h3", "### ", " ###\n\n", NULL },
	{ "h4", "#### ", " ####\n\n", NULL },
	{ "h5", "##### ", " #####\n\n", NULL },
	{ "h6", "###### ", " ######\n\n", NULL },
	{ "p", NULL, "\n\n", NULL },
	{ "blockquote", "> ", "\n\n", NULL },
	{ "strong", "**", "**", NULL },
	{ "em", "*", "*", NULL },
	{ "code", "`", "`", NULL },
	{ "ins", "{++", "++}", NULL },
	{ "del", "{--", "--}", NULL },
	{ "mark", "{==", "==}", NULL },
	{ "br", "\\", NULL, NULL },
};


static int match_element(const char * e) {
	F(i, (int) (sizeof(elements) / sizeof(elements[0]))) {
		if (!strcmp(elements[i].element, e)) {
			return i;
		}
	}

	return -1;
}


static yxml_ret_t xml_parse_elem(text_buffer * out, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;

	int i = match_element(x->elem);

	if (i >= 0) {
		if (elements[i].prefix) {
			text_buffer_append_printf(out, "%s", elements[i].prefix);
		}

		if (elements[i].f) {
			ret = elements[i].f(out, &ch, x);

			if (elements[i].suffix) {
				text_buffer_append_printf(out, "%s", elements[i].suffix);
			}

			*source = ch;
			return ret;
		}
	}

	text_buffer * buf = text_buffer_new(0);

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				ch++;
				ret = xml_parse_elem(out, &ch, x);

				if (ret < 0) {
					goto exit;
				}

				break;

			case YXML_ATTRSTART:
				buf->len = 0;
				break;

			case YXML_ATTRVAL:
				text_buffer_append_printf(buf, "%s", x->data);
				break;

			case YXML_ATTREND:
				// Do something with this?
				break;

			case YXML_CONTENT:

				// May be one or several characters
				if (strcmp(x->elem, "html") && strcmp(x->elem, "head") && strcmp(x->elem, "body") && strcmp(x->elem, "head")) {
					// Ignore extra stuff and whitespace, at least for now
					append_content(out, x);
				}

				break;

			case YXML_ELEMEND:
				goto leave;
				break;

			default:
				break;
		}

		ch++;
	}

leave:

	if (i >= 0 && elements[i].suffix) {
		text_buffer_append_printf(out, "%s", elements[i].suffix);
	}

exit:
	text_buffer_free(buf, 1);
	*source = ch;
	return ret;
}


int mmd_import_html(text_buffer * source_buffer) {
	char * ch = NULL;
	int result = 0;

	// Prepare to parse XML
	yxml_ret_t ret;
	yxml_t * x = malloc(sizeof(yxml_t) + kYXML_BUFSIZE);
	yxml_init(x, x + 1, kYXML_BUFSIZE);

	// We need a new textbuffer for the output
	text_buffer * output = text_buffer_new(0);
	// text_buffer * metadata = text_buffer_new(0);

	// Temporary storage
	text_buffer * buf = text_buffer_new(0);

	// Does this look like HTML?
	if (strncmp("<!DOCTYPE html>", source_buffer->text, 15)) {
		fprintf(stderr, "Error: Source text does not begin with '<!DOCTYPE html>'\n");
		goto cleanup;
	}

	// Trick yxml into thinking this is xml
	char * xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
	ch = xml;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			// This should not happen
			fprintf(stderr, "preamble error\n");
			goto cleanup;
		}

		ch++;
	}

	// Now, process the actual source text
	ch = source_buffer->text;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			break;
		} else {
			switch (ret) {
				case YXML_ELEMSTART:
					ch++;
					ret = xml_parse_elem(output, &ch, x);

					if (ret < 0) {
						goto cleanup;
					}

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

	if (ret < 0) {
		fprintf(stderr, "XML error parsing as HTML %d at EOF\n", ret);
	} else {
		source_buffer->len = 0;
		text_buffer_append_text(source_buffer, output->text, output->len);

		result = 1;
	}

	text_buffer_free(output, 1);
	text_buffer_free(buf, 1);

	return result;
}

