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
	size_t		class;
	size_t		class_len;

	size_t		content;
	size_t		content_len;

	size_t		href;
	size_t		href_len;

	size_t		id;
	size_t		id_len;

	size_t		name;
	size_t		name_len;

	size_t		title;
	size_t		title_len;
} attr_index;


typedef yxml_ret_t (*xml_parse_func)(text_buffer *, text_buffer *, char **, yxml_t *);
typedef void (*custom_func)(text_buffer *, text_buffer *, text_buffer *, text_buffer *, attr_index *, yxml_t *);


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


enum content_actions {
	CONTENT_IGNORE	= 1 << 0,
	CONTENT_LEAD	= 1 << 1,
};


static yxml_ret_t xml_parse_elem(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);

static yxml_ret_t parse_div(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);
// static yxml_ret_t parse_meta(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);
// static yxml_ret_t parse_ol(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);
// static yxml_ret_t parse_ul(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);
// static yxml_ret_t parse_pre(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);
// static yxml_ret_t parse_a(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x);

static void custom_link(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x);
static void custom_meta(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x);

static html_element elements[] = {
	{ "html",		0,	NULL,		CONTENT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "head",		0,	NULL,		CONTENT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "title",		1,	"title:\t",	0,				"  ",		0,	NULL,		NULL,		NULL },
	{ "meta",		1,	NULL,		CONTENT_IGNORE,	NULL,		0,	NULL,		NULL, 		&custom_meta },
	{ "body",		2,	NULL,		CONTENT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "div",		2,	NULL,		0,				NULL,		0,	NULL,		&parse_div,	NULL },
	{ "h1",			3,	"# ",		0,				" #",		0,	NULL,		NULL,		NULL },
	{ "h2",			3,	"## ",		0,				" ##",		0,	NULL,		NULL,		NULL },
	{ "h3",			3,	"### ",		0,				" ###",		0,	NULL,		NULL,		NULL },
	{ "h4",			3,	"#### ",	0,				" ####",	0,	NULL,		NULL,		NULL },
	{ "h5",			3,	"##### ",	0,				" #####",	0,	NULL,		NULL,		NULL },
	{ "h6",			3,	"###### ",	0,				" ######",	0,	NULL,		NULL,		NULL },
	{ "p",			2,	NULL,		0,				NULL,		0,	NULL,		NULL,		NULL },
	{ "blockquote",	2,	"> ",		CONTENT_IGNORE,	NULL,		0,	"> ",		NULL,		NULL },
	{ "pre",		2,	"\t",		CONTENT_LEAD,	NULL,		0,	"\t",		NULL,		NULL },
	{ "hr",			2,	NULL,		0,				"***",		0,	NULL,		NULL,		NULL },
	{ "ul",			2,	NULL,		CONTENT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "ol",			2,	NULL,		CONTENT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "li",			1,	NULL,		0,				NULL,		0,	"\t",		NULL,		NULL },
	{ "a",			0,	NULL,		CONTENT_IGNORE,	NULL,		0,	NULL,		NULL,		&custom_link },
	{ "strong",		0,	"**",		0,				"**",		0,	NULL,		NULL,		NULL },
	{ "em",			0,	"*",		0,				"*",		0,	NULL,		NULL,		NULL },
	{ "code",		0,	"`",		CONTENT_LEAD,	"`",		0,	NULL,		NULL,		NULL },
	{ "ins",		0,	"{++",		0,				"++}",		0,	NULL,		NULL,		NULL },
	{ "del",		0,	"{--",		0,				"--}",		0,	NULL,		NULL,		NULL },
	{ "mark",		0,	"{==",		0,				"==}",		0,	NULL,		NULL,		NULL },
	{ "br",			0,	NULL,		0,				"\\",		0,	NULL,		NULL,		NULL },
	{ "table",		2,	NULL,		CONTENT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "tbody",		0,	NULL,		CONTENT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "tr",			1,	NULL,		CONTENT_IGNORE,	" |  ",		0,	NULL,		NULL,		NULL },
	{ "th",			0,	"| ",		0,				NULL,		0,	NULL,		NULL,		NULL },
	{ "td",			0,	"| ",		0,				NULL,		0,	NULL,		NULL,		NULL },
	{ "dl",			2,	NULL,		CONTENT_IGNORE,	NULL,		0,	NULL,		NULL,		NULL },
	{ "dt",			1,	NULL,		0,				NULL,		0,	NULL,		NULL,		NULL },
	{ "dd",			1,	":\t",		0,				NULL,		0,	"\t",		NULL,		NULL },
};


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
			case '[':
			case ']':
			case '\\':
			case '*':
			case '_':
				text_buffer_append_c(out, '\\');
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


static void custom_meta(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x) {
	if (0 && lead && content && x) {}

	if (index->name_len) {
		text_buffer_append_printf(out, "%.*s:\t", index->name_len, &attr->text[index->name]);
		// Include 2 spaces for line break when falling back to plain Markdown without metadata
		text_buffer_append_printf(out, "%.*s  ", index->content_len, &attr->text[index->content]);
		out->padding = 0;
	}
}


static void custom_link(text_buffer * out, text_buffer * lead, text_buffer * attr, text_buffer * content, attr_index * index, yxml_t * x) {
	if (0 && lead && x) {}

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


/// Ignore this element (and its children)
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

	lead->len = lead_len;
	lead_pad(out, lead, 2);

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

	lead_pad(out, lead, 2);

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
							// TODO: Refactor this
							ret = parse_endnotes(out, lead, &ch, x, type);

							// ret = parse_ignore(out, lead, &ch, x);
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


// static yxml_ret_t parse_ul(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
// 	char * ch = *source;
// 	yxml_ret_t ret = 0;
// 	size_t lead_len = lead->len;

// 	int i = match_element(x->elem);

// 	if (i >= 0) {
// 		lead_pad(out, lead, elements[i].pre_pad);
// 	}

// 	while (*ch != '\0') {
// 		ret = yxml_parse(x, *ch);

// 		if (ret < 0) {
// 			goto exit;
// 		}

// 		switch (ret) {
// 			case YXML_ELEMSTART:
// 				text_buffer_pad(out, 1);

// 				if (!strcmp(x->elem, "li")) {
// 					text_buffer_append_text(out, lead->text, lead->len);
// 					text_buffer_append_text(out, "* ", 2);
// 					out->padding = 2;
// 				}

// 				ch++;
// 				ret = xml_parse_elem(out, lead, &ch, x);
// 				text_buffer_pad(out, 1);

// 				if (ret < 0) {
// 					goto exit;
// 				}

// 				break;

// 			case YXML_ELEMEND:
// 				goto leave;
// 				break;

// 			default:
// 				break;
// 		}

// 		ch++;
// 	}

// leave:

// 	if (i >= 0) {
// 		// text_buffer_pad(out, elements[i].post_pad);
// 	}

// exit:
// 	lead->len = lead_len;
// 	*source = ch;
// 	return ret;
// }


// static yxml_ret_t parse_ol(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
// 	char * ch = *source;
// 	yxml_ret_t ret = 0;
// 	size_t lead_len = lead->len;

// 	int i = match_element(x->elem);

// 	if (i >= 0) {
// 		lead_pad(out, lead, elements[i].pre_pad);
// 	}

// 	int c = 1;

// 	while (*ch != '\0') {
// 		ret = yxml_parse(x, *ch);

// 		if (ret < 0) {
// 			goto exit;
// 		}

// 		switch (ret) {
// 			case YXML_ELEMSTART:
// 				text_buffer_pad(out, 1);

// 				if (!strcmp(x->elem, "li")) {
// 					text_buffer_append_text(out, lead->text, lead->len);
// 					text_buffer_append_printf(out, "%d. ", c++);
// 					out->padding = 2;
// 				}

// 				ch++;
// 				ret = xml_parse_elem(out, lead, &ch, x);

// 				text_buffer_pad(out, 1);

// 				if (ret < 0) {
// 					goto exit;
// 				}

// 				break;

// 			case YXML_ELEMEND:
// 				goto leave;
// 				break;

// 			default:
// 				break;
// 		}

// 		ch++;
// 	}

// leave:

// 	if (i >= 0) {
// 		// text_buffer_pad(out, elements[i].post_pad);
// 	}

// exit:
// 	lead->len = lead_len;
// 	*source = ch;
// 	return ret;
// }


// static yxml_ret_t parse_pre(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
// 	char * ch = *source;
// 	yxml_ret_t ret = 0;
// 	size_t lead_len = lead->len;

// 	text_buffer * buf = text_buffer_new(0);

// 	while (*ch != '\0') {
// 		ret = yxml_parse(x, *ch);

// 		if (ret < 0) {
// 			goto exit;
// 		}

// 		switch (ret) {
// 			case YXML_ELEMSTART:
// 				ch++;
// 				ret = parse_pre(out, lead, &ch, x);

// 				if (ret < 0) {
// 					goto exit;
// 				}

// 				break;

// 			case YXML_CONTENT:

// 				// May be one or several characters
// 				text_buffer_append_printf(out, "%s", x->data);

// 				if (x->data[0] == '\n') {
// 					text_buffer_append_text(out, lead->text, lead->len);
// 				}

// 				break;

// 			case YXML_ELEMEND:
// 				goto leave;
// 				break;

// 			default:
// 				break;
// 		}

// 		ch++;
// 	}

// leave:

// exit:
// 	out->padding = 1;
// 	lead->len = lead_len;
// 	text_buffer_free(buf, 1);
// 	*source = ch;
// 	return ret;
// }


static yxml_ret_t xml_parse_elem(text_buffer * out, text_buffer * lead, char ** source, yxml_t * x) {
	char * ch = *source;
	yxml_ret_t ret = 0;
	size_t lead_len = lead->len;

	int i = match_element(x->elem);


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
			ret = elements[i].parse(out, lead, &ch, x);

			if (elements[i].suffix) {
				text_buffer_append_printf(out, "%s", elements[i].suffix);
			}

			text_buffer_pad(out, elements[i].post_pad);

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
				if (!strcmp(x->attr, "href")) {
					index.href = attr->len;
				} else if (!strcmp(x->attr, "title")) {
					index.title = attr->len;
				} else if (!strcmp(x->attr, "id")) {
					index.id = attr->len;
				} else if (!strcmp(x->attr, "class")) {
					index.class = attr->len;
				} else if (!strcmp(x->attr, "name")) {
					index.name = attr->len;
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
				} else if (!strcmp(x->attr, "title")) {
					index.title_len = attr->len - index.title;
				} else if (!strcmp(x->attr, "id")) {
					index.id_len = attr->len - index.id;
				} else if (!strcmp(x->attr, "class")) {
					index.class_len = attr->len - index.class;
				} else if (!strcmp(x->attr, "name")) {
					index.name_len = attr->len - index.name;
				} else if (!strcmp(x->attr, "content")) {
					index.content_len = attr->len - index.content;
				}

				break;

			case YXML_CONTENT:

				// May be one or several characters
				if (i >= 0 && (elements[i].handle_content & CONTENT_IGNORE)) {
					// Store for possible use
					text_buffer_append_printf(content, "%s", x->data);
				} else if (elements[i].handle_content & CONTENT_LEAD) {
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
				if (i >= 0 && elements[i].custom_out) {
					elements[i].custom_out(out, lead, attr, content, &index, x);
				} else {
					out->padding = 0;

					if (i >= 0) {
						if (elements[i].suffix) {
							text_buffer_append_printf(out, "%s", elements[i].suffix);
						}

						text_buffer_pad(out, elements[i].post_pad);
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

