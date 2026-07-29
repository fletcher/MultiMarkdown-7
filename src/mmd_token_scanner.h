/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_token_scanner.h

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


#ifndef MMD_TOKEN_SCANNER_LIBMULTIMARKDOWN7_H
#define MMD_TOKEN_SCANNER_LIBMULTIMARKDOWN7_H

enum table_alignment {
	ALIGN_LEFT		= 1 << 0,
	ALIGN_RIGHT		= 1 << 1,
	ALIGN_CENTER	= 1 << 2,
	ALIGN_WRAP		= 1 << 3,
};


/// Scan text for MultiMarkdown tokens
int mmd_token_scan(Scanner * s, uint32_t options);

/// Scan text for CriticMarkup tokens
int criticmarkup_token_scan(Scanner * s);

abbr_def * scan_ref_abbr(const char * text, size_t len);
link_def * scan_ref_link(const char * text, size_t len);
attr * scan_link_attributes(const char * text, size_t len, size_t * scanned);
endnote_def * scan_ref_endnote(const char * text, size_t len);

int scan_table_alignment(const char * text);

size_t scan_html(const char * text);
size_t scan_url(const char * text);
size_t scan_email(const char * text);

size_t scan_metadata(const char * text, size_t len, meta ** m);

char * copy_fence_language_specificer(const char * text);
void scan_toc_range(const char * text, int * min, int * max);

size_t is_valid_ref_link(const char * text, size_t len);

#endif
