/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file text_buffer.h

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


#ifndef TEXT_BUFFER_LIBMULTIMARKDOWN7_H
#define TEXT_BUFFER_LIBMULTIMARKDOWN7_H


typedef struct {
	char 	*	text;
	size_t		len;
	size_t		capacity;
	int			padding;
} text_buffer;


text_buffer * text_buffer_new(size_t capacity);

void text_buffer_free(text_buffer * b, int free_text);

/// Append array of characters
void text_buffer_append_text(text_buffer * b, const char * text, size_t len);

/// Append single character
void text_buffer_append_c(text_buffer * b, char c);

/// Append a formatted string (this will not be as fast)
void text_buffer_append_printf(text_buffer * b, const char * format, ...);

/// Replace a section of existing string with new string
void text_buffer_replace_range(text_buffer * b, size_t pos, size_t len, const char * replacement, size_t replacement_len);

/// Insert new string into existing string
void text_buffer_insert_text(text_buffer * b, size_t pos, const char * text, size_t text_len);

/// Insert new string prior to existing string
void text_buffer_prepend_text(text_buffer * b, const char * text, size_t text_len);

/// Read from file stream into a buffer
text_buffer * buffer_file(FILE * in, size_t capacity);
text_buffer * buffer_filename(const char * fname, size_t capacity);
void text_buffer_delete_range(text_buffer * b, size_t pos, size_t len);
void text_buffer_delete_bom(text_buffer * b);

/// Pad text with newlines
void text_buffer_pad(text_buffer * b, short n);

/// Remove trailing whitespace
void text_buffer_trim_trailing_whitespace(text_buffer * b);

/// Convert trailing CRLF to LF
void text_buffer_fix_trailing_newline(text_buffer * b);

/// Remove trailing CR or LF
void text_buffer_trim_trailing_newline(text_buffer * b);

/// Replace occurences of target with replacement
void text_buffer_replace_string(text_buffer * b, const char * target, const char * replacement);

#endif
