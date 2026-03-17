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
#include "mmd_node_pool.h"
#include "read_ctx.h"
#include "mmd_span_parser.h"
#include "yxml.h"

#define F(i,n) for(int i= 0;i<n;i++)

#define kYXML_BUFSIZE 4096

typedef yxml_ret_t (*xml_parse_func)(text_buffer *, text_buffer *, char **, yxml_t *);

typedef struct {
	const char *	element;
	short			pre_pad;
	const char *	prefix;
	short			ignore_content;
	const char *	suffix;
	short			post_pad;
	const char *	lead;
	xml_parse_func	f;
} html_element;


enum link_type {
	TYPE_PLAIN,
	TYPE_FOOTNOTE,
	TYPE_GLOSSARY,
	TYPE_CITATION,
	TYPE_IGNORE
};


static yxml_ret_t xml_parse_elem(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);

static yxml_ret_t parse_div(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);
static yxml_ret_t parse_meta(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);
static yxml_ret_t parse_ol(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);
static yxml_ret_t parse_ul(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);
static yxml_ret_t parse_pre(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);
static yxml_ret_t parse_a(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);


static html_element elements[] = {
	{ "html", 0, NULL, 0, NULL, 0, NULL, NULL },
	{ "head", 0, NULL, 0, NULL, 0, NULL, NULL },
	{ "title", 0, "title:\t", 0, "  ", 1, NULL, NULL },
	{ "meta", 0, NULL, 0, NULL, 0, NULL, &parse_meta },
	{ "div", 0, NULL, 0, NULL, 0, NULL, &parse_div },
	{ "h1", 3, "# ", 0, " #", 2, NULL, NULL },
	{ "h2", 3, "## ", 0, " ##", 2, NULL, NULL },
	{ "h3", 3, "### ", 0, " ###", 2, NULL, NULL },
	{ "h4", 3, "#### ", 0, " ####", 2, NULL, NULL },
	{ "h5", 3, "##### ", 0, " #####", 2, NULL, NULL },
	{ "h6", 3, "###### ", 0, " ######", 2, NULL, NULL },
	{ "p", 2, NULL, 0, "", 2, NULL, NULL },
	{ "blockquote", 2, "> ", 0, "", 2, "> ", NULL },
	{ "pre", 2, "\t", 0, "", 2, "\t", &parse_pre },
	{ "hr", 2, NULL, 0, "***", 2, NULL, NULL },
	{ "ul", 2, NULL, 0, "", 2, NULL, &parse_ul },
	{ "ol", 2, NULL, 0, "", 2, NULL, &parse_ol },
	{ "li", 1, NULL, 0, NULL, 1, "\t", NULL },
	{ "a", 0, NULL, 0, NULL, 0, NULL, &parse_a },
	{ "strong", 0, "**", 0, "**", 0, NULL, NULL },
	{ "em", 0, "*", 0, "*", 0, NULL, NULL },
	{ "code", 0, "`", 0, "`", 0, NULL, NULL },
	{ "ins", 0, "{++", 0, "++}", 0, NULL, NULL },
	{ "del", 0, "{--", 0, "--}", 0, NULL, NULL },
	{ "mark", 0, "{==", 0, "==}", 0, NULL, NULL },
	{ "br", 0, NULL, 0, "\\", 0, NULL, NULL },
	{ "table", 2, NULL, 1, NULL, 2, NULL, NULL },
	{ "tbody", 0, NULL, 1, NULL, 2, NULL, NULL },
	{ "tr", 1, NULL, 1, " |  ", 1, NULL, NULL },
	{ "th", 0, "| ", 0, NULL, 0, NULL, NULL },
	{ "td", 0, "| ", 0, NULL, 0, NULL, NULL },
	{ "dl", 2, NULL, 1, NULL, 2, NULL, NULL },
	{ "dt", 1, NULL, 0, NULL, 1, NULL, NULL },
	{ "dd", 1, ":\t", 0, NULL, 1, "\t", NULL },
};


static int match_element(const char * e) {
	F(i, (int) (sizeof(elements) / sizeof(elements[0]))) {
		if (!strcmp(elements[i].element, e)) {
			return i;
		}
	}

	return -1;
}


static void append_content(text_buffer * out, text_buffer * lead, yxml_t * x) {
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

			case '\n':
				if (out->padding < 1) {
					text_buffer_pad(out, 1);
					text_buffer_append_text(out, lead->text, lead->len);
				}

				break;

			default:
				text_buffer_append_printf(out, "%s", x->data);
				break;
		}
	}
}


static yxml_ret_t parse_meta(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

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
					out->padding = 1;
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
	lead->len = lead_len;
	text_buffer_free(buf, 1);
	*source = ch;
	return ret;
}


static yxml_ret_t parse_ignore(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
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
				ret = parse_ignore(out, lead, &ch, x);

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


static yxml_ret_t parse_endnotes(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, enum link_type type) {
	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;
	text_buffer_append_text(lead, "\t", 1);

	text_buffer * content = text_buffer_new(0);

	char marker = '\0';

	switch (type) {
		case TYPE_CITATION:
			marker = '#';
			break;

		case TYPE_FOOTNOTE:
			marker = '^';
			break;

		case TYPE_GLOSSARY:
			marker = '?';
			break;

		default:
			marker = ' ';
			break;
	}

	int c = 1;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				ch++;

				if (type == TYPE_GLOSSARY) {
					c++;
					text_buffer_append_printf(out, "[%c TODO: Fix this (parse li special)%s]: ", marker, content->text);
				} else {
					text_buffer_append_printf(out, "[%c%d]: ", marker, c++);
				}

				out->padding = 2;

				ret = xml_parse_elem(out, lead, &ch, x);

				if (ret < 0) {
					goto exit;
				}

				break;

			case YXML_CONTENT:
				text_buffer_append_printf(content, "%s", x->data);
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

	text_buffer_pad(out, 2);

exit:
	lead->len = lead_len;
	text_buffer_free(content, 1);
	*source = ch;
	return ret;
}


static yxml_ret_t parse_div(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	text_buffer * buf = text_buffer_new(0);

	text_buffer_pad(out, 2);

	enum link_type type = TYPE_PLAIN;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				ch++;

				switch (type) {
					case TYPE_CITATION:
					case TYPE_FOOTNOTE:
					case TYPE_GLOSSARY:
						if (!strcmp("ol", x->elem)) {
							ret = parse_endnotes(out, lead, &ch, x, type);
						} else {
							ret = parse_ignore(out, lead, &ch, x);
						}

						break;

					default:
						ret = xml_parse_elem(out, lead, &ch, x);
						break;
				}

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
						ret = parse_ignore(out, lead, &ch, x);
						text_buffer_pad(out, 2);
						text_buffer_append_text(out, "{{TOC}}\n", 9);
						out->padding = 1;
						goto exit;
					} else if (!strcmp(buf->text, "citations")) {
						type = TYPE_CITATION;
					} else if (!strcmp(buf->text, "footnotes")) {
						type = TYPE_FOOTNOTE;
					} else if (!strcmp(buf->text, "glossary")) {
						type = TYPE_GLOSSARY;
					}
				}

				break;

			case YXML_CONTENT:

				// May be one or several characters
				if (strcmp(x->elem, "html") && strcmp(x->elem, "head") && strcmp(x->elem, "body") && strcmp(x->elem, "head")) {
					// Ignore extra stuff and whitespace, at least for now
					append_content(out, lead, x);

					if (x->data[0] == '\n') {
						out->padding = 1;
					} else {
						out->padding = 0;
					}
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

	text_buffer_pad(out, 2);

exit:
	lead->len = lead_len;
	text_buffer_free(buf, 1);
	*source = ch;
	return ret;
}


static yxml_ret_t parse_ul(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	int i = match_element(x->elem);

	if (i >= 0) {
		text_buffer_pad(out, elements[i].pre_pad);
	}

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				text_buffer_pad(out, 1);

				if (!strcmp(x->elem, "li")) {
					text_buffer_append_text(out, lead->text, lead->len);
					text_buffer_append_text(out, "* ", 2);
					out->padding = 2;
				}

				ch++;
				ret = xml_parse_elem(out, lead, &ch, x);
				text_buffer_pad(out, 1);

				if (ret < 0) {
					goto exit;
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

	if (i >= 0) {
		text_buffer_pad(out, elements[i].post_pad);
	}

exit:
	lead->len = lead_len;
	*source = ch;
	return ret;
}


static yxml_ret_t parse_ol(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	int i = match_element(x->elem);

	if (i >= 0) {
		text_buffer_pad(out, elements[i].pre_pad);
	}

	int c = 1;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				text_buffer_pad(out, 1);

				if (!strcmp(x->elem, "li")) {
					text_buffer_append_text(out, lead->text, lead->len);
					text_buffer_append_printf(out, "%d. ", c++);
					out->padding = 2;
				}

				ch++;
				ret = xml_parse_elem(out, lead, &ch, x);

				text_buffer_pad(out, 1);

				if (ret < 0) {
					goto exit;
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

	if (i >= 0) {
		text_buffer_pad(out, elements[i].post_pad);
	}

exit:
	lead->len = lead_len;
	*source = ch;
	return ret;
}


static yxml_ret_t parse_pre(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	text_buffer * buf = text_buffer_new(0);

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				ch++;
				ret = parse_pre(out, lead, &ch, x);

				if (ret < 0) {
					goto exit;
				}

				break;

			case YXML_CONTENT:

				// May be one or several characters
				text_buffer_append_printf(out, "%s", x->data);

				if (x->data[0] == '\n') {
					text_buffer_append_text(out, lead->text, lead->len);
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
	out->padding = 1;
	lead->len = lead_len;
	text_buffer_free(buf, 1);
	*source = ch;
	return ret;
}


static yxml_ret_t parse_a(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	text_buffer * buf = text_buffer_new(0);
	text_buffer * content = text_buffer_new(0);

	size_t href = -1;
	size_t href_len = 0;

	size_t title = -1;
	size_t title_len = 0;

	size_t id = -1;
	size_t class = -1;

	enum link_type type = TYPE_PLAIN;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				ch++;
				// ret = xml_parse_elem(out, lead, &ch, x);
				ret = parse_ignore(out, lead, &ch, x);

				if (ret < 0) {
					goto exit;
				}

				break;

			case YXML_ATTRSTART:
				if (!strcmp(x->attr, "href")) {
					href = buf->len;
				} else if (!strcmp(x->attr, "title")) {
					title = buf->len;
				} else if (!strcmp(x->attr, "id")) {
					id = buf->len;
				} else if (!strcmp(x->attr, "class")) {
					class = buf->len;
				}

				break;

			case YXML_ATTRVAL:
				text_buffer_append_printf(buf, "%s", x->data);
				break;

			case YXML_ATTREND:
				if (!strcmp(x->attr, "href")) {
					href_len = buf->len - href;
				} else if (!strcmp(x->attr, "title")) {
					title_len = buf->len - title;
				} else if (!strcmp(x->attr, "id")) {
					if (!strncmp("cnref:", &buf->text[id], 6)) {
						type = TYPE_CITATION;
					} else if (!strncmp("fnref:", &buf->text[id], 6)) {
						type = TYPE_FOOTNOTE;
					} else if (!strncmp("gnref:", &buf->text[id], 6)) {
						type = TYPE_GLOSSARY;
					}
				} else if (!strcmp(x->attr, "class")) {
					if (!strncmp("reverse", &buf->text[class], 7)) {
						type = TYPE_IGNORE;
					}
				}

				break;

			case YXML_CONTENT:
				text_buffer_append_printf(content, "%s", x->data);
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

	switch (type) {
		case TYPE_FOOTNOTE:
			text_buffer_append_printf(out, "[^%.*s]", href_len - 4, &buf->text[href + 4]);
			break;

		case TYPE_GLOSSARY:
			text_buffer_append_printf(out, "[?%s]", content->text);
			break;

		case TYPE_CITATION:
			text_buffer_append_printf(out, "[#%.*s]", href_len - 4, &buf->text[href + 4]);
			break;

		case TYPE_PLAIN:
			if (title_len > 0) {
				text_buffer_append_printf(out, "[%s](%.*s \"%.*s\")", content->text, href_len, &buf->text[href], title_len, &buf->text[title]);
			} else {
				if (!strncmp(content->text, &buf->text[href], href_len)) {
					// Automatic Link
					text_buffer_append_printf(out, "<%s>", content->text);
				} else if (!strncmp(&buf->text[href], "mailto:", 7) && !strcmp(content->text, &buf->text[href + 7])) {
					// Mailto automatic link
					text_buffer_append_printf(out, "<%s>", content->text);
				} else if (buf->text[href] == '#') {
					char * id = html_id_from_text(content->text, content->len, false);

					if (!strcmp(id, &buf->text[href + 1])) {
						text_buffer_append_printf(out, "[%s][]", content->text);
					} else {
						text_buffer_append_printf(out, "[%s](%.*s)", content->text, href_len, &buf->text[href]);
					}

					free(id);
				} else {
					text_buffer_append_printf(out, "[%s](%.*s)", content->text, href_len, &buf->text[href]);
				}
			}

			break;

		case TYPE_IGNORE:
			break;
	}

exit:
	lead->len = lead_len;
	text_buffer_free(buf, 1);
	text_buffer_free(content, 1);
	*source = ch;
	return ret;
}


static yxml_ret_t xml_parse_elem(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	int i = match_element(x->elem);

	// text_buffer_append_printf(out, "<%s>", x->elem);

	if (i >= 0) {
		text_buffer_pad(out, elements[i].pre_pad);

		if (elements[i].prefix) {
			text_buffer_append_printf(out, "%s", elements[i].prefix);
			out->padding = 2;
		}

		if (elements[i].lead) {
			text_buffer_append_printf(lead, "%s", elements[i].lead);
		}

		if (elements[i].f) {
			ret = elements[i].f(out, lead, &ch, x);

			if (elements[i].suffix) {
				text_buffer_append_printf(out, "%s", elements[i].suffix);
			}

			text_buffer_pad(out, elements[i].post_pad);

			lead->len = lead_len;
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
				ret = xml_parse_elem(out, lead, &ch, x);

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
				if (i >= 0 && elements[i].ignore_content) {

				} else {
					// May be one or several characters
					if (strcmp(x->elem, "html") && strcmp(x->elem, "head") && strcmp(x->elem, "body") && strcmp(x->elem, "head")) {
						// Ignore extra stuff and whitespace, at least for now
						if (x->data[0] != '\n') {
							out->padding = 0;
						}

						append_content(out, lead, x);
					}
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

	if (i >= 0) {
		if (elements[i].suffix) {
			text_buffer_append_printf(out, "%s", elements[i].suffix);
			out->padding = 0;
		}

		text_buffer_pad(out, elements[i].post_pad);
	}

	// text_buffer_append_printf(out, "</>");

exit:
	lead->len = lead_len;
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

	// Lead in for nested structures
	text_buffer * lead = text_buffer_new(0);

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
					ret = xml_parse_elem(output, lead, &ch, x);

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
	text_buffer_free(lead, 1);

	return result;
}

