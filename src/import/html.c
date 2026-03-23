/**

	libMultiMarkdown7 -- C parser for Markdown with additional features and multiple output formats.

	@file html.c

	@brief Parses HTML and returns plain text MultiMarkdown.  Uses yxml to
	 parse the underlying HTML, treating it as XML.  To do this, we have to
	 include a "fake" XML declaration, and we have to accept any named entity
	 as valid (e.g. &whatever;), instead of being limited to the 5 predefined
	 named entities in XML (&lt;, &gt;, &amp;, &apos;, &quot;).  Otherwise, a
	 successful parse ensures that we were given valid XML, but not that we
	 were given valid HTML.


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


#include <ctype.h>
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

typedef struct {
	size_t		alt;
	size_t		alt_len;

	size_t		class;
	size_t		class_len;

	size_t		colspan;
	size_t		colspan_len;

	size_t		content;
	size_t		content_len;

	size_t		href;
	size_t		href_len;

	size_t		id;
	size_t		id_len;

	size_t		name;
	size_t		name_len;

	size_t		src;
	size_t		src_len;

	size_t		title;
	size_t		title_len;
} attr_index;


typedef yxml_ret_t (*xml_parse_func)(text_buffer *, text_buffer *, char **, yxml_t *, const char *, int);
typedef void (*custom_func)(text_buffer *, text_buffer *, text_buffer *, text_buffer *, attr_index *, yxml_t *, const char *);


typedef struct {
	const char *	element;
	short			pre_pad;
	const char *	prefix;
	short			handle_content;
	const char *	suffix;
	short			post_pad;
	const char *	lead;
	xml_parse_func	parse;
	custom_func		custom_out;
} html_element;


enum link_type {
	TYPE_PLAIN,
	TYPE_FOOTNOTE,
	TYPE_GLOSSARY,
	TYPE_CITATION,
	TYPE_IGNORE
};


enum options {
	OPT_IGNORE			= 1 << 0,
	OPT_IGNORE_CHILDREN	= 1 << 1,
	OPT_LEAD			= 1 << 2,
	OPT_FLATTEN			= 1 << 3,
	OPT_SKIP_CHILD		= 1 << 4,
};


static yxml_ret_t xml_parse_elem(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, const char * parent, int idx);

static yxml_ret_t parse_div(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, const char * parent, int idx);
static yxml_ret_t parse_li(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, const char * parent, int idx);

static yxml_ret_t parse_pre(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, const char * parent, int idx);


static void custom_link(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self);
static void custom_img(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self);
static void custom_meta(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self);
static void custom_span(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self);
static void custom_thead(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self);
static void custom_td(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self);
static void custom_abbr(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self);
static void custom_hx(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self);


static html_element elements[] = {
	{ "html",		0,	NULL,		OPT_IGNORE,	NULL,		2,	NULL,		NULL,		NULL },
	{ "head",		0,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "title",		1,	"title:\t",	0,			"  ",		0,	NULL,		NULL,		NULL },
	{ "meta",		1,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL, 		&custom_meta },
	{ "body",		2,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "div",		2,	NULL,		0,			NULL,		0,	NULL,		&parse_div,	NULL },
	{ "h1",			3,	NULL,		OPT_IGNORE | OPT_IGNORE_CHILDREN,	NULL,	0,	NULL,	NULL,	&custom_hx },
	{ "h2",			3,	NULL,		OPT_IGNORE | OPT_IGNORE_CHILDREN,	NULL,	0,	NULL,	NULL,	&custom_hx },
	{ "h3",			3,	NULL,		OPT_IGNORE | OPT_IGNORE_CHILDREN,	NULL,	0,	NULL,	NULL,	&custom_hx },
	{ "h4",			3,	NULL,		OPT_IGNORE | OPT_IGNORE_CHILDREN,	NULL,	0,	NULL,	NULL,	&custom_hx },
	{ "h5",			3,	NULL,		OPT_IGNORE | OPT_IGNORE_CHILDREN,	NULL,	0,	NULL,	NULL,	&custom_hx },
	{ "h6",			3,	NULL,		OPT_IGNORE | OPT_IGNORE_CHILDREN,	NULL,	0,	NULL,	NULL,	&custom_hx },
	{ "p",			2,	NULL,		0,			NULL,		1,	NULL,		NULL,		NULL },
	{ "blockquote",	2,	"> ",		OPT_IGNORE,	NULL,		0,	"> ",		NULL,		NULL },
	{ "pre",		2,	"\t",		OPT_LEAD,	NULL,		0,	"\t",		&parse_pre,	NULL },
	{ "hr",			2,	NULL,		0,			"***",		0,	NULL,		NULL,		NULL },
	{ "ul",			2,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "ol",			2,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "li",			1,	NULL,		0,			NULL,		0,	"\t",		&parse_li,	NULL },
	{ "a",			0,	NULL,		OPT_IGNORE | OPT_IGNORE_CHILDREN,	NULL,	0,	NULL,	NULL,	&custom_link },
	{ "img",		0,	NULL,		OPT_IGNORE | OPT_IGNORE_CHILDREN,	NULL,	0,	NULL,	NULL,	&custom_img },
	{ "figure",		2,	NULL,		OPT_IGNORE | OPT_FLATTEN,	NULL,	0,	NULL,	NULL,	&custom_img },
	{ "figcaption",	1,	NULL,		0,			NULL,		0,	NULL,		NULL,		NULL },
	{ "strong",		0,	"**",		0,			"**",		0,	NULL,		NULL,		NULL },
	{ "em",			0,	"*",		0,			"*",		0,	NULL,		NULL,		NULL },
	{ "code",		0,	"`",		OPT_LEAD,	"`",		0,	NULL,		NULL,		NULL },
	{ "ins",		0,	"{++",		0,			"++}",		0,	NULL,		NULL,		NULL },
	{ "del",		0,	"{--",		0,			"--}",		0,	NULL,		NULL,		NULL },
	{ "mark",		0,	"{==",		0,			"==}",		0,	NULL,		NULL,		NULL },
	{ "br",			0,	NULL,		0,			"\\",		0,	NULL,		NULL,		NULL },
	{ "table",		2,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "tbody",		2,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "thead",		0,	NULL,		OPT_IGNORE | OPT_IGNORE_CHILDREN,	NULL,	0,	NULL,	NULL,	&custom_thead },
	{ "tr",			1,	"| ",		OPT_IGNORE,	"  ",		0,	NULL,		NULL,		NULL },
	{ "th",			0,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL,		&custom_td },
	{ "td",			0,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL,		&custom_td },
	{ "dl",			2,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "dt",			1,	NULL,		0,			NULL,		0,	NULL,		NULL,		NULL },
	{ "dd",			1,	":\t",		0,			NULL,		0,	"\t",		NULL,		NULL },
	{ "span",		0,	NULL,		OPT_IGNORE,	NULL,		0,	NULL,		NULL,		&custom_span },
	{ "abbr",		0,	NULL,		OPT_IGNORE | OPT_IGNORE_CHILDREN,	NULL,	0,	NULL,	NULL,	&custom_abbr },
};


static void yxml_error(yxml_ret_t ret, yxml_t * x) {
	fprintf(stderr, "Error %d parsing HTML at line %u byte %" PRIu64 ".\n", ret, x->line, x->byte);
}


static int match_element(const char * e) {
	F(i, (int) (sizeof(elements) / sizeof(elements[0]))) {
		if (!strcmp(elements[i].element, e)) {
			return i;
		}
	}

	return -1;
}


static void lead_pad(text_buffer * out, text_buffer * lead, int n) {
	while (n > out->padding) {
		text_buffer_append_c(out, '\n');
		text_buffer_append_text(out, lead->text, lead->len);
		out->padding++;
	}
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
			case '\\':
				text_buffer_append_c(out, '\\');
				text_buffer_append_c(out, x->data[0]);
				break;

			case '[':
			case ']':
			case '*':
			case '_':
				if (out->text[out->len] != '\\') {
					text_buffer_append_c(out, '\\');
				}

				text_buffer_append_c(out, x->data[0]);
				break;

			case '\n':
				lead_pad(out, lead, 1);
				break;

			default:
				text_buffer_append_printf(out, "%s", x->data);
				break;
		}
	}
}


static void custom_meta(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self) {
	if (0 && lead && content && x && self) {}

	if (index->name_len) {
		text_buffer_append_printf(out, "%.*s:\t", index->name_len, &attr->text[index->name]);
		// Include 2 spaces for line break when falling back to plain Markdown without metadata
		text_buffer_append_printf(out, "%.*s  ", index->content_len, &attr->text[index->content]);
		out->padding = 0;
	}
}


static int add_label(text_buffer * out, text_buffer * content, const char * id, size_t id_len) {
	int result = 0;

	if (id_len) {
		char * html_id = html_id_from_text(content->text, content->len, false);

		if (strncmp(id, html_id, id_len)) {
			text_buffer_append_printf(out, " [%.*s]", id_len, id);

			result = 1;
		}

		free(html_id);
	}

	return result;
}


static void custom_hx(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self) {
	if (0 && lead && content && index && x && out && attr && self) {}

	// If there is a newline, this must be a Setext header
	// Otherwise default to ATX
	int nl = 0;
	F(i, (int)content->len) {
		if (char_is_line_ending(content->text[i])) {
			nl = 1;
			break;
		}
	}

	if (nl) {
		text_buffer_append_text(out, content->text, content->len);

		add_label(out, content, &attr->text[index->id], index->id_len);

		if (!strcmp(self, "h1")) {
			text_buffer_append_text(out, "\n========", 9);
		} else {
			text_buffer_append_text(out, "\n--------", 9);
		}
	} else {
		F(i, (int)(self[1] - '0')) {
			text_buffer_append_c(out, '#');
		}

		text_buffer_append_c(out, ' ');

		text_buffer_append_text(out, content->text, content->len);

		add_label(out, content, &attr->text[index->id], index->id_len);

		text_buffer_append_c(out, ' ');

		F(i, (int)(self[1] - '0')) {
			text_buffer_append_c(out, '#');
		}
	}

	out->padding = 0;
}


static void custom_link(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self) {
	if (0 && lead && x && self) {}

	out->padding = 0;

	// What sort of link are we dealing with?
	enum link_type type = TYPE_PLAIN;

	if (index->id_len) {
		if (!strncmp("cnref:", &attr->text[index->id], 6)) {
			type = TYPE_CITATION;
		} else if (!strncmp("fnref:", &attr->text[index->id], 6)) {
			type = TYPE_FOOTNOTE;
		} else if (!strncmp("gnref:", &attr->text[index->id], 6)) {
			type = TYPE_GLOSSARY;
		}
	}

	if (index->class_len) {
		if (!strncmp("reverse", &attr->text[index->class], 7)) {
			type = TYPE_IGNORE;
		}
	}

	// Based on type, append output
	switch (type) {
		case TYPE_IGNORE:
			break;

		case TYPE_CITATION:
			text_buffer_append_printf(out, "[#%.*s]", index->href_len - 4, &attr->text[index->href + 4]);
			break;

		case TYPE_FOOTNOTE:
			text_buffer_append_printf(out, "[^%.*s]", index->href_len - 4, &attr->text[index->href + 4]);
			break;

		case TYPE_GLOSSARY:
			text_buffer_append_printf(out, "[?%.*s]", content->len, content->text);
			break;

		case TYPE_PLAIN:
			if (index->title_len) {
				// If there's a title, we need everything (but could be reference link)
				text_buffer_append_printf(out, "[%.*s](%.*s \"%.*s\")", content->len, content->text,
										  index->href_len, &attr->text[index->href], index->title_len, &attr->text[index->title]);
			} else {
				if (!strncmp(content->text, &attr->text[index->href], index->href_len)) {
					// Automatic link
					text_buffer_append_printf(out, "<%.*s>", content->len, content->text);
				} else if (!strncmp(&attr->text[index->href], "mailto:", 7) && !strncmp(content->text, &attr->text[index->href + 7], content->len)) {
					// Mailto automatic link
					text_buffer_append_printf(out, "<%.*s>", content->len, content->text);
				} else if (attr->text[index->href] == '#') {
					char * id = html_id_from_text(content->text, content->len, false);

					if (!strcmp(id, &attr->text[index->href + 1])) {
						text_buffer_append_printf(out, "[%.*s][]", content->len, content->text);
					} else {
						text_buffer_append_printf(out, "[%.*s](%.*s)", content->len, content->text, index->href_len, &attr->text[index->href]);
					}

					free(id);
				} else {
					text_buffer_append_printf(out, "[%.*s](%.*s)", content->len, content->text, index->href_len, &attr->text[index->href]);
				}
			}

			break;
	}
}


static void custom_img(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self) {
	if (0 && lead && x && content && self) {}

	out->padding = 0;

	// TODO: If there is an `id` attribute in a MMD generated image, then it was a reference style image with `id` as the label
	// We could reverse this if desired...

	if (content->len) {
		text_buffer_append_printf(out, "![%.*s](%.*s ", content->len, content->text, index->src_len, &attr->text[index->src]);
	} else {
		text_buffer_append_printf(out, "![%.*s](%.*s ", index->alt_len, &attr->text[index->alt], index->src_len, &attr->text[index->src]);
	}

	if (index->title_len) {
		text_buffer_append_printf(out, "title=\"%.*s\" ", index->title_len, &attr->text[index->title]);
	}

	text_buffer_append_printf(out, ")");
}


static void custom_span(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self) {
	if (0 && lead && x && content && self) {}

	if (index->class_len) {
		if (!strcmp("math", &attr->text[index->class])) {
			text_buffer_append_text(out, "\\", 1);
			text_buffer_append_printf(out, "%.*s", content->len, content->text);
			text_buffer_replace_range(out, out->len - 1, 0, "\\", 1);
		} else if (!strcmp("critic comment", &attr->text[index->class])) {
			text_buffer_append_text(out, "{>>", 3);
			text_buffer_append_printf(out, "%.*s", content->len, content->text);
			text_buffer_append_text(out, "<<}", 3);
		} else {
			// TODO: I might need to process this text further for special characters
			text_buffer_append_printf(out, "%.*s", content->len, content->text);
		}
	}
}


static void custom_abbr(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self) {
	if (0 && lead && x && content && self) {}

	if (content->len && index->title_len) {
		if (content->text[0] == '(') {
			// This was the first usage of an abbreviation
			if (!strncmp(&attr->text[index->title], &out->text[out->len - index->title_len - 1], index->title_len)) {
				text_buffer_replace_range(out, out->len - index->title_len - 1, index->title_len, NULL, 0);
				text_buffer_append_printf(out, "[>(%.*s) %.*s]", content->len - 2, &content->text[1], index->title_len, &attr->text[index->title]);
				return;
			}
		} else {
			// This was a subsequent usage
		}
	}

	text_buffer_append_printf(out, "%.*s", content->len, content->text);
}


static void custom_thead(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self) {
	if (0 && lead && x && content && attr && index && self) {}

	text_buffer_append_printf(out, "%.*s", content->len, content->text);

	size_t offset = 0;

	while (offset < content->len) {
		switch (content->text[offset]) {
			case '\n':
			case '\r':
				break;

			case ' ':
			case '\t':
			case '|':
				text_buffer_append_c(out, content->text[offset]);
				break;

			default:
				text_buffer_append_c(out, '-');
				break;
		}

		offset++;
	}

	text_buffer_append_c(out, '\n');
	out->padding = 2;
}


static void custom_td(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x, const char * self) {
	if (0 && lead && x && content && self) {}

	text_buffer_append_printf(out, "%.*s", content->len, content->text);

	if (index->colspan_len) {
		char buffer[10] = {0};
		strncat(buffer, &attr->text[index->colspan], index->colspan_len);
		int c = atoi(buffer);

		F(i, c) {
			text_buffer_append_c(out, '|');
		}
	} else {
		text_buffer_append_c(out, '|');
	}
}


/// Ignore this element (and its children)
static yxml_ret_t parse_ignore(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, const char * parent) {
	if (0 && parent) {}

	char * ch = *source;
	yxml_ret_t ret = 0;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			yxml_error(ret, x);
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMEND:
				goto leave;
				break;

			case YXML_ELEMSTART:
				ch++;
				ret = parse_ignore(out, lead, &ch, x, NULL);

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


/// Ignore this element but process children
static yxml_ret_t parse_skip(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, const char * parent, int idx) {
	if (0 && parent && idx) {}

	char * ch = *source;
	yxml_ret_t ret = 0;

	const char * self = x->elem;
	int c = 0;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			yxml_error(ret, x);
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMEND:
				goto leave;
				break;

			case YXML_ELEMSTART:
				ch++;
				ret = xml_parse_elem(out, lead, &ch, x, self, ++c);

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

	const char * self = x->elem;

	int c = 0;

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

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			yxml_error(ret, x);
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				ch++;
				c++;

				if (type == TYPE_GLOSSARY) {
					text_buffer_append_printf(out, "[%c", marker);
					// Look ahead for term
					char * cur = ch;

					while (*cur != '>') {
						cur++;
					}

					cur++;

					while (char_is_whitespace_or_line_ending(*cur)) {
						cur++;
					}

					while (*cur != ':') {
						text_buffer_append_c(out, *cur++);
					}

					text_buffer_append_text(out, "]: ", 3);
				} else {
					text_buffer_append_printf(out, "[%c%d]: ", marker, c);
				}

				out->padding = 2;

				ret = parse_skip(out, lead, &ch, x, self, c);
				// ret = xml_parse_elem(out, lead, &ch, x, self, c);

				text_buffer_append_printf(out, "\n");
				out->padding = 2;

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

	lead->len = lead_len;
	lead_pad(out, lead, 2);

exit:
	lead->len = lead_len;
	text_buffer_free(content, 1);
	*source = ch;
	return ret;
}


static yxml_ret_t parse_div(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, const char * parent, int idx) {
	if (0 && parent && idx) {}

	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	const char * self = x->elem;
	int c = 0;

	text_buffer * buf = text_buffer_new(0);

	lead_pad(out, lead, 2);

	enum link_type type = TYPE_PLAIN;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			yxml_error(ret, x);
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
							// TODO: Refactor this
							ret = parse_endnotes(out, lead, &ch, x, type);

							// ret = parse_ignore(out, lead, &ch, x, self);
						} else {
							ret = parse_ignore(out, lead, &ch, x, self);
						}

						break;

					default:
						ret = xml_parse_elem(out, lead, &ch, x, self, ++c);
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
						ret = parse_ignore(out, lead, &ch, x, self);
						lead_pad(out, lead, 2);
						text_buffer_append_text(out, "{{TOC}}", 7);
						out->padding = 0;
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

	lead->len = lead_len;
	lead_pad(out, lead, 2);

exit:
	lead->len = lead_len;
	text_buffer_free(buf, 1);
	*source = ch;
	return ret;
}


static yxml_ret_t parse_li(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, const char * parent, int idx) {
	if (0 && parent && idx) {}

	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	const char * self = x->elem;
	int i = match_element(self);

	int c = 0;

	text_buffer * content = text_buffer_new(0);

	// Customize prefix
	if (!strcmp("ol", parent)) {
		text_buffer_append_printf(out, "%d. ", idx);
	} else {
		text_buffer_append_printf(out, "* ");
	}

	out->padding = 2;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			yxml_error(ret, x);
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				ch++;
				ret = xml_parse_elem(out, lead, &ch, x, self, ++c);

				if (ret < 0) {
					goto exit;
				}

				break;

			case YXML_CONTENT:

				// May be one or several characters
				if (i >= 0 && (elements[i].handle_content & OPT_IGNORE)) {
					// Store for possible use
					text_buffer_append_printf(content, "%s", x->data);
				} else if (elements[i].handle_content & OPT_LEAD) {
					text_buffer_append_printf(out, "%s", x->data);

					if (x->data[0] == '\n') {
						text_buffer_append_text(out, lead->text, lead->len);
					}
				} else {
					if (x->data[0] != '\n') {
						out->padding = 0;
					}

					append_content(out, lead, x);
				}

				break;

			case YXML_ELEMEND:
				out->padding = 0;

				if (i >= 0) {
					if (elements[i].suffix) {
						text_buffer_append_printf(out, "%s", elements[i].suffix);
					}

					// text_buffer_pad(out, elements[i].post_pad);
					lead_pad(out, lead, elements[i].post_pad);
				}

				// text_buffer_append_printf(out, "</>");

				goto exit;
				break;

			default:
				break;
		}

		ch++;
	}

exit:
	lead->len = lead_len;
	text_buffer_free(content, 1);
	*source = ch;
	return ret;
}


static yxml_ret_t parse_pre(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, const char * parent, int idx) {
	if (0 && idx) {}

	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	text_buffer * buf = text_buffer_new(0);

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			yxml_error(ret, x);
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				ch++;
				ret = parse_pre(out, lead, &ch, x, parent, 0);

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


static yxml_ret_t xml_parse_elem(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x, const char * parent, int idx) {
	if (0 && parent) {}

	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	const char * self = x->elem;
	int i = match_element(self);

	// Count children
	int c = 0;

	// Count depth
	int d = 0;

	if (i >= 0) {
		lead_pad(out, lead, elements[i].pre_pad);

		if (elements[i].prefix) {
			text_buffer_append_printf(out, "%s", elements[i].prefix);
			out->padding = 2;
		}

		// text_buffer_append_printf(out, "<%s>", x->elem);

		if (elements[i].lead) {
			text_buffer_append_printf(lead, "%s", elements[i].lead);
		}

		if (elements[i].parse) {
			ret = elements[i].parse(out, lead, &ch, x, parent, idx);

			if (elements[i].suffix) {
				text_buffer_append_printf(out, "%s", elements[i].suffix);
			}

			// text_buffer_pad(out, elements[i].post_pad);
			lead_pad(out, lead, elements[i].post_pad);

			lead->len = lead_len;
			*source = ch;
			return ret;
		}
	} else {
		// text_buffer_append_printf(out, "<%s>", x->elem);
	}

	text_buffer * attr = text_buffer_new(0);
	text_buffer * content = text_buffer_new(0);

	// Store indices for attributes as needed
	attr_index index = {0};

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			yxml_error(ret, x);
			goto exit;
		}

		switch (ret) {
			case YXML_ELEMSTART:
				if (i >= 0 && (elements[i].handle_content & OPT_FLATTEN)) {
					// We keep parsing here rather than descending
					d++;
					self = x->elem;

					// Add prefix
					int i = match_element(self);

					if (i >= 0) {
						if (elements[i].prefix) {
							text_buffer_append_printf(content, "%s", elements[i].prefix);
							content->padding = 2;
						}
					}
				} else {
					ch++;

					if (i >= 0 && (elements[i].handle_content & OPT_IGNORE_CHILDREN)) {
						ret = xml_parse_elem(content, lead, &ch, x, self, ++c);
					} else if (i >= 0 && (elements[i].handle_content & OPT_SKIP_CHILD)) {
						ret = parse_skip(out, lead, &ch, x, self, ++c);
					} else {
						ret = xml_parse_elem(out, lead, &ch, x, self, ++c);
					}
				}

				if (ret < 0) {
					goto exit;
				}

				break;

			case YXML_ATTRSTART:
				if (!strcmp(x->attr, "href")) {
					index.href = attr->len;
				} else if (!strcmp(x->attr, "alt")) {
					index.alt = attr->len;
				} else if (!strcmp(x->attr, "title")) {
					index.title = attr->len;
				} else if (!strcmp(x->attr, "id")) {
					index.id = attr->len;
				} else if (!strcmp(x->attr, "class")) {
					index.class = attr->len;
				} else if (!strcmp(x->attr, "colspan")) {
					index.colspan = attr->len;
				} else if (!strcmp(x->attr, "name")) {
					index.name = attr->len;
				} else if (!strcmp(x->attr, "src")) {
					index.src = attr->len;
				} else if (!strcmp(x->attr, "content")) {
					index.content = attr->len;
				}

				break;

			case YXML_ATTRVAL:
				text_buffer_append_printf(attr, "%s", x->data);
				break;

			case YXML_ATTREND:
				if (!strcmp(x->attr, "href")) {
					index.href_len = attr->len - index.href;
				} else if (!strcmp(x->attr, "alt")) {
					index.alt_len = attr->len - index.alt;
				} else if (!strcmp(x->attr, "title")) {
					index.title_len = attr->len - index.title;
				} else if (!strcmp(x->attr, "id")) {
					index.id_len = attr->len - index.id;
				} else if (!strcmp(x->attr, "class")) {
					index.class_len = attr->len - index.class;
				} else if (!strcmp(x->attr, "colspan")) {
					index.colspan_len = attr->len - index.colspan;
				} else if (!strcmp(x->attr, "name")) {
					index.name_len = attr->len - index.name;
				} else if (!strcmp(x->attr, "src")) {
					index.src_len = attr->len - index.src;
				} else if (!strcmp(x->attr, "content")) {
					index.content_len = attr->len - index.content;
				}

				break;

			case YXML_CONTENT:

				// May be one or several characters
				if (i >= 0 && (elements[i].handle_content & OPT_FLATTEN)) {
					int i = match_element(self);

					if (i >= 0 && (elements[i].handle_content & OPT_IGNORE)) {
						// Now we ignore it for real
					} else if (i >= 0 && (elements[i].handle_content & OPT_LEAD)) {
						text_buffer_append_printf(content, "%s", x->data);

						if (x->data[0] == '\n') {
							text_buffer_append_text(content, lead->text, lead->len);
						}
					} else {
						if (x->data[0] != '\n') {
							content->padding = 0;
						}

						append_content(content, lead, x);
					}
				} else {
					if (i >= 0 && (elements[i].handle_content & OPT_IGNORE)) {
						// Store for possible use
						text_buffer_append_printf(content, "%s", x->data);
					} else if (i >= 0 && (elements[i].handle_content & OPT_LEAD)) {
						text_buffer_append_printf(out, "%s", x->data);

						if (x->data[0] == '\n') {
							text_buffer_append_text(out, lead->text, lead->len);
						}
					} else {
						if (x->data[0] != '\n') {
							out->padding = 0;
						}

						append_content(out, lead, x);
					}
				}

				break;

			case YXML_ELEMEND:
				if (i >= 0 && (elements[i].handle_content & OPT_FLATTEN)) {
					if (d) {
						d--;

						// Add suffix
						int i = match_element(self);
						self = x->elem;

						if (i >= 0) {
							if (elements[i].suffix) {
								text_buffer_append_printf(content, "%s", elements[i].suffix);
								content->padding = 2;
							}
						}

						break;
					}
				}

				if (i >= 0 && elements[i].custom_out) {
					elements[i].custom_out(out, lead, attr, content, &index, x, self);
				} else {
					out->padding = 0;

					if (i >= 0) {
						if (elements[i].suffix) {
							text_buffer_append_printf(out, "%s", elements[i].suffix);
						}

						// text_buffer_pad(out, elements[i].post_pad);
						lead_pad(out, lead, elements[i].post_pad);
					}
				}

				// text_buffer_append_printf(out, "</>");

				goto exit;
				break;

			default:
				break;
		}

		ch++;
	}

exit:
	lead->len = lead_len;
	text_buffer_free(attr, 1);
	text_buffer_free(content, 1);
	*source = ch;
	return ret;
}


int mmd_import_html(text_buffer * source_buffer) {
	char * ch = NULL;
	int result = 0;

	int c = 0;

	// Prepare to parse XML
	yxml_ret_t ret;
	yxml_t * x = malloc(sizeof(yxml_t) + kYXML_BUFSIZE);
	yxml_init(x, x + 1, kYXML_BUFSIZE);


	// We need a new textbuffer for the output
	text_buffer * output = text_buffer_new(0);
	output->padding = 2;


	// Lead in for nested structures
	text_buffer * lead = text_buffer_new(0);

	// Does this look like HTML?
	if (strncmp("<!DOCTYPE html", source_buffer->text, 14)) {
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
			yxml_error(ret, x);
			goto cleanup;
		}

		ch++;
	}

	// Try to clean things up (HTML is not as strict as XML/XHTML)
	html_cleanup(source_buffer);

	// Now, process the actual source text
	ch = source_buffer->text;

	while (*ch != '\0') {
		ret = yxml_parse(x, *ch);

		if (ret < 0) {
			yxml_error(ret, x);
			break;
		} else {
			switch (ret) {
				case YXML_ELEMSTART:
					ch++;
					ret = xml_parse_elem(output, lead, &ch, x, NULL, ++c);

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
		// Error
		fprintf(stderr, "XML error parsing as HTML %d at EOF\n", ret);
	} else {
		// Success
		source_buffer->len = 0;
		text_buffer_append_text(source_buffer, output->text, output->len);

		result = 1;
	}

	text_buffer_free(output, 1);
	text_buffer_free(lead, 1);

	return result;
}

static const char * voids = " area, base, br, col, embed, hr, img, input, link, meta, source, track, wbr,";

/// Perform some basic cleaning of HTML source to improve ability to parse it
void html_cleanup(text_buffer * source) {
	size_t offset = 0;

	char buffer[128] = {0};

	while (offset < source->len) {
		switch (source->text[offset]) {
			case '<': {
				// Ensure lowercase tags for consistency
				size_t tag_start = ++offset;

				buffer[0] = ' ';	// Start with space
				buffer[1] = '\0';

				// Lowercase start tags to ensure consistency
				while (char_is_alphanumeric(source->text[offset])) {
					source->text[offset] = tolower(source->text[offset]);
					strncat(buffer, &source->text[offset], 1);
					offset++;
				}

				if ((offset - tag_start)) {
					// Append
					strncat(buffer, ",", 1);	// End with comma

					if (strstr(voids, buffer)) {
						// This is a void element -- ensure it is self-closing

						while (source->text[offset] != '\0') {
							if (source->text[offset] == '>') {
								if (source->text[offset - 1] != '/') {
									// Insert solidus
									text_buffer_replace_range(source, offset, 0, "/", 1);
									offset++;
								}

								break;
							}

							offset++;
						}
					}
				} else {
					switch (source->text[offset]) {
						case '/':
							++offset;

							// Lowercase end tags to ensure consistency
							while (char_is_alphanumeric(source->text[offset])) {
								source->text[offset] = tolower(source->text[offset]);
								strncat(buffer, &source->text[offset], 1);
								offset++;
							}

							break;

						default:
							break;
					}
				}
			}

			break;

			default:
				break;
		}

		offset++;
	}
}
