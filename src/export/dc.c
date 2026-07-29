/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file dc.c

	@brief Dublin Core Metadata shortcuts


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

#include "dc.h"


static char * dc_term[] = {
	[DC_CREATOR]		= "creator",
	[DC_IDENTIFIER]		= "identifier",
	[DC_LANGUAGE]		= "language",
	[DC_TITLE]			= "title",
	[DC_PUBLISHER]		= "publisher",
	[DC_DATE]			= "date",
};


void dc_write_term(text_buffer * out, enum dc_metadata term, const char * value, const char * id, const char * lead) {
	if (id) {
		if (lead) {
			text_buffer_append_printf(out, "<dc:%s id=\"%s\">%s%s</dc:%s>\n", dc_term[term], id, lead, value, dc_term[term]);
		} else {
			text_buffer_append_printf(out, "<dc:%s id=\"%s\">%s</dc:%s>\n", dc_term[term], id, value, dc_term[term]);
		}
	} else {
		if (lead) {
			text_buffer_append_printf(out, "<dc:%s>%s%s</dc:%s>\n", dc_term[term], lead, value, dc_term[term]);
		} else {
			text_buffer_append_printf(out, "<dc:%s>%s</dc:%s>\n", dc_term[term], value, dc_term[term]);
		}
	}
}


static char * language_code[] = {
	[LANGUAGE_EN] = "en",
	[LANGUAGE_ES] = "es",
	[LANGUAGE_DE] = "de",
	[LANGUAGE_FR] = "fr",
	[LANGUAGE_NL] = "nl",
	[LANGUAGE_SV] = "sv",
	[LANGUAGE_HE] = "he",
};

void dc_write_language(text_buffer * out, enum language lang) {
	dc_write_term(out, DC_LANGUAGE, language_code[lang], NULL, NULL);
}

