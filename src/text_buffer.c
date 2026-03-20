/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file text_buffer.c

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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text_buffer.h"

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	#include <windows.h>
#endif


#ifdef TEST
	#include "CuTest.h"
#endif


#define kBUFFERSIZE 4096				// How many bytes to read at a time?

/// Read from file stream into a buffer
text_buffer * buffer_file(FILE * in, size_t capacity) {
	if (capacity < kBUFFERSIZE + 1) {
		capacity = kBUFFERSIZE * 4;
	}

	size_t bytes, size = 0;

	char * text = malloc(sizeof(char) * capacity);

	while ((bytes = fread(&text[size], sizeof(char), kBUFFERSIZE, in)) > 0) {
		size += bytes;

		while (size + kBUFFERSIZE + 1 > capacity) {
			char * new = realloc(text, capacity * 2);

			if (new == NULL) {
				fprintf(stderr, "Failed to realloc() while buffering file\n");
				// Reallocation failed
				free(text);
				return NULL;
			}

			text = new;
			capacity *= 2;
		}
	}

	text_buffer * buffer = malloc(sizeof(text_buffer));

	if (buffer) {
		buffer->text = text;
		buffer->len = size;
		buffer->capacity = capacity;
		buffer->text[size] = '\0';
	}

	return buffer;
}


text_buffer * buffer_filename(const char * fname, size_t capacity) {
	text_buffer * r = NULL;

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	int wchars_num = MultiByteToWideChar(CP_UTF8, 0, fname, -1, NULL, 0);
	wchar_t * wstr = malloc(sizeof(wchar_t) * wchars_num);
	MultiByteToWideChar(CP_UTF8, 0, fname, -1, wstr, wchars_num);

	FILE * in = _wfopen(wstr, L"rb");

	if (in) {
		r = buffer_file(in, capacity);
		fclose(in);
	}

	free(wstr);
#else
	FILE * in = fopen(fname, "r");

	if (in) {
		r = buffer_file(in, capacity);
		fclose(in);
	}

#endif

	return r;
}


/*
 * The following section came from:
 *
 *	http://lists-archives.org/mingw-users/12649-asprintf-missing-vsnprintf-
 *		behaving-differently-and-_vsncprintf-undefined.html
 *
 * and
 *
 *	http://groups.google.com/group/jansson-users/browse_thread/thread/
 *		76a88d63d9519978/041a7d0570de2d48?lnk=raot
 */

// Some operating systems do not supply vasprintf() -- standardize on this
// replacement from:
//		https://github.com/esp8266/Arduino/issues/1954
static int my_vasprintf(char ** strp, const char * fmt, va_list ap) {
	va_list ap2;
	va_copy(ap2, ap);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	char * tmp = NULL;
	int size = vsnprintf(tmp, 0, fmt, ap2);
#else
	char tmp[1];
	int size = vsnprintf(tmp, 1, fmt, ap2);
#endif

	if (size <= 0) {
		return size;
	}

	va_end(ap2);
	size += 1;
	*strp = (char *)malloc(size * sizeof(char));
	return vsnprintf(*strp, size, fmt, ap);
}


#define kMinimumCapacity	8192
#define kGrowthMultiplier		2

text_buffer * text_buffer_new(size_t capacity) {
	text_buffer * b = malloc(sizeof(text_buffer));

	if (b) {
		size_t start_capacity = (capacity) ? capacity : kMinimumCapacity;

		b->text = malloc(start_capacity + 1);

		if (!b->text) {
			free(b);
			return NULL;
		}

		b->len = 0;
		b->capacity = start_capacity;
		b->text[0] = '\0';
		b->padding = 0;
	}

	return b;
}


void text_buffer_free(text_buffer * b, int free_text) {
	if (b) {
		if (free_text) {
			free(b->text);
		}

		free(b);
	}
}


static void text_buffer_ensure_capacity(text_buffer * b, size_t new_capacity) {
	if (b) {
		if (b->capacity > new_capacity + 1) {
			return;
		}

		size_t target = new_capacity + 1;
		new_capacity = b->capacity;

		while (target > new_capacity) {
			new_capacity *= kGrowthMultiplier;
		}

		char * new_text = realloc(b->text, new_capacity);

		if (new_text) {
			b->text = new_text;
			b->capacity = new_capacity;
		}
	}
}


/// Append array of characters
void text_buffer_append_text(text_buffer * b, const char * text, size_t len) {
	if (b && text && len) {
		text_buffer_ensure_capacity(b, b->len + len);

		memcpy((void *)(b->text + b->len), text, len);
		b->len += len;
		b->text[b->len] = '\0';
	}
}


/// Append single character
void text_buffer_append_c(text_buffer * b, char c) {
	if (b) {
		text_buffer_ensure_capacity(b, b->len + 1);

		b->text[b->len++] = c;
		b->text[b->len] = '\0';
	}
}


/// Append a formatted string (this will not be as fast)
void text_buffer_append_printf(text_buffer * b, const char * format, ...) {
	if (b && format) {
		va_list args;
		va_start(args, format);

		char * formatted_string = NULL;
		int valid = my_vasprintf(&formatted_string, format, args);

		if ((valid > 0)) {
			if (formatted_string) {
				text_buffer_append_text(b, formatted_string, strlen(formatted_string));
				free(formatted_string);
			}
		}

		va_end(args);
	}
}


/// Replace a section of existing string with new string
void text_buffer_replace_range(text_buffer * b, size_t pos, size_t len, const char * replacement, size_t replacement_len) {
	if (b && (replacement || !replacement_len) && (len || replacement_len)) {
		text_buffer_ensure_capacity(b, b->len + replacement_len - len);

		// Shift "tail" portion of existing string after the excised portion
		memmove((void *)(b->text + pos + replacement_len), b->text + pos + len, b->len - pos - len);

		// Insert new string at designated position
		memcpy((void *)(b->text + pos), replacement, replacement_len);

		// Adjust b->len
		b->len += replacement_len - len;
		b->text[b->len] = '\0';
	}
}


void text_buffer_insert_text(text_buffer * b, size_t pos, const char * text, size_t text_len) {
	text_buffer_replace_range(b, pos, 0, text, text_len);
}


void text_buffer_prepend_text(text_buffer * b, const char * text, size_t text_len) {
	text_buffer_replace_range(b, 0, 0, text, text_len);
}


void text_buffer_delete_range(text_buffer * b, size_t pos, size_t len) {
	if (b && len) {
		// Shift "tail" portion of existing string
		memmove((void *)(b->text + pos), b->text + pos + len, b->len - pos - len);

		// Adjust b->len
		b->len -= len;
		b->text[b->len] = '\0';
	}
}


void text_buffer_delete_bom(text_buffer * b) {
	int skip = 0;

	// Strip UTF-8 BOM
	if (strncmp(b->text, "\xef\xbb\xbf", 3) == 0) {
		skip += 3;
	}

	// Strip UTF-16 BOMs
	if (strncmp(&b->text[skip], "\xef\xff", 2) == 0) {
		skip += 2;
	}

	if (strncmp(&b->text[skip], "\xfe\xff", 2) == 0) {
		skip += 2;
	}

	text_buffer_delete_range(b, 0, skip);
}


/// Pad text with newlines
void text_buffer_pad(text_buffer * b, short n) {
	while (n > b->padding) {
		text_buffer_append_c(b, '\n');
		b->padding++;
	}
}


/// Remove trailing whitespace
void text_buffer_trim_trailing_whitespace(text_buffer * b) {
	if (b) {
		while (b->len) {
			switch (b->text[b->len - 1]) {
				case ' ':
				case '\t':
					b->len--;
					b->text[b->len] = '\0';
					break;

				default:
					return;
			}
		}
	}
}


/// Convert trailing CRLF to LF
void text_buffer_fix_trailing_newline(text_buffer * b) {
	if (b) {
		if (b->len > 1) {
			if (b->text[b->len - 2] == '\r' && b->text[b->len - 1] == '\n') {
				b->len -= 1;
				b->text[b->len] = '\0';
				b->text[b->len - 1] = '\n';
			}
		}
	}
}


/// Remove trailing CR or LF
void text_buffer_trim_trailing_newline(text_buffer * b) {
	if (b) {
		while (b->len) {
			switch (b->text[b->len - 1]) {
				case '\r':
				case '\n':
					b->len--;
					b->text[b->len] = '\0';
					break;

				default:
					return;
			}
		}
	}
}


/// Replace occurences of target with replacement
void text_buffer_replace_string(text_buffer * b, const char * target, const char * replacement) {
	if (b && target && replacement) {
		size_t offset = 0;
		size_t t_len = strlen(target);
		size_t r_len = strlen(replacement);

		char * needle = strstr(&b->text[offset], target);

		while (needle) {
			offset = needle - b->text;
			text_buffer_replace_range(b, offset, t_len, replacement, r_len);
			offset += r_len - t_len;
			needle = strstr(&b->text[offset], target);
		}
	}
}


