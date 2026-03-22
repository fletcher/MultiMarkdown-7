/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_utilities.h

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


#ifndef MMD_UTILITIES_LIBMULTIMARKDOWN7_H
#define MMD_UTILITIES_LIBMULTIMARKDOWN7_H

int table_has_caption(mmd_node * t);
char * table_label(mmd_node * t, const char * text);

void custom_seed_rand(void);
uint16_t xorshift16(uint16_t x);


/// strdup() not available on all platforms
char * my_strdup(const char * source);


/// strndup not available on all platforms
char * my_strndup(const char * source, size_t n);


char * uuid_string_from_bits(unsigned char * raw);
char * uuid_new(void);

/// Open file for reading regardless of OS
FILE * flex_fopen(const char * fname);

/// Cross-platform dirname()
char * mmd_dirname(const char * path);


/// Check text for a specified char
int text_contains_char(const char * text, size_t len, char target);

#endif
