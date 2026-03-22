/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_utilities.c

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


#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	#include <windows.h>
#else
	#include <libgen.h>
#endif


#include "libMultiMarkdown7.h"

#include "mmd_node_pool.h"
#include "read_ctx.h"
#include "mmd_span_parser.h"
#include "mmd_utilities.h"


int table_has_caption(mmd_node * t) {
	if (t && t->next && t->next->type == BLOCK_PARA) {
		int result = 1;
		t = t->next->content;

		if (t && t->type == TOKEN_PAIR_BRACKET) {
			t = t->next->next;

			if (t && t->type == TOKEN_PAIR_BRACKET) {
				result++;
				t = t->next->next;
			}

			if (t == NULL) {
				return result;
			}

			if (t && ((t->type == TOKEN_NL) || (t->type == TOKEN_LINEBREAK))) {
				return result;
			}
		}
	}

	return 0;
}


char * table_label(mmd_node * t, const char * text) {
	if (t) {
		mmd_node * b = t->next;

		switch (table_has_caption(t)) {
			case 2:

				// This table has a caption and a label -- use label
				if (b->content && b->content->next && b->content->next->next) {
					return html_id_from_text(&text[b->start + b->content->next->next->start], b->len - b->content->next->next->start, false);
				}

			case 1:
				// This table has a caption -- use caption
				return html_id_from_text(&text[t->next->start], t->next->len, false);

			default:
				break;
		}
	}

	return NULL;
}


// http://stackoverflow.com/questions/322938/recommended-way-to-initialize-srand
// http://www.concentric.net/~Ttwang/tech/inthash.htm
static unsigned long mix(unsigned long a, unsigned long b, unsigned long c) {
	a = a - b;
	a = a - c;
	a = a ^ (c >> 13);
	b = b - c;
	b = b - a;
	b = b ^ (a << 8);
	c = c - a;
	c = c - b;
	c = c ^ (b >> 13);
	a = a - b;
	a = a - c;
	a = a ^ (c >> 12);
	b = b - c;
	b = b - a;
	b = b ^ (a << 16);
	c = c - a;
	c = c - b;
	c = c ^ (b >> 5);
	a = a - b;
	a = a - c;
	a = a ^ (c >> 3);
	b = b - c;
	b = b - a;
	b = b ^ (a << 10);
	c = c - a;
	c = c - b;
	c = c ^ (b >> 15);
	return c;
}


void custom_seed_rand(void) {
	// Seed random number generator
	// This is not a "cryptographically secure" random seed,
	// but good enough for an EPUB id....
	unsigned long seed = mix(clock(), time(NULL), clock());
	srand((unsigned int)seed);
}


/// http://www.retroprogramming.com/2017/07/xorshift-pseudorandom-numbers-in-z80.html
/// Quickly generate 16 bit random number from a given seed or prior number
uint16_t xorshift16(uint16_t x) {
	x ^= x << 7;
	x ^= x >> 9;
	x ^= x << 8;

	return x;
}


/// strdup() not available on all platforms
char * my_strdup(const char * source) {
	if (source == NULL) {
		return NULL;
	}

	char * result = malloc(strlen(source) + 1);

	if (result) {
		strcpy(result, source);
	}

	return result;
}


/// strndup not available on all platforms
char * my_strndup(const char * source, size_t n) {
	if (source == NULL) {
		return NULL;
	}

	size_t len = 0;
	char * result;
	const char * test = source;

	// strlen is too slow if strlen(source) >> n
	for (len = 0; len < n; ++len) {
		if (*test == '\0') {
			break;
		}

		test++;
	}

	result = malloc(len + 1);

	if (result) {
		memcpy(result, source, len);
		result[len] = '\0';
	}

	return result;
}


char * uuid_string_from_bits(unsigned char * raw) {
	char * result = malloc(37);

	snprintf(result, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			 raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7],
			 raw[8], raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15] );

	return result;
}


#define SETBIT(a, n) (a[n/CHAR_BIT] |= (1<<(n % CHAR_BIT)))
#define CLEARBIT(a, n) (a[n/CHAR_BIT] &= ~(1<<(n % CHAR_BIT)))


char * uuid_new(void) {
	unsigned char raw[16];

	// Get 128 bits of random goodness
	for (int i = 0; i < 16; ++i) {
		raw[i] = rand() % 256;
	}

//	Need to set certain bits for v4 compliance
	CLEARBIT(raw, 52);
	CLEARBIT(raw, 53);
	SETBIT(raw, 54);
	CLEARBIT(raw, 55);
	CLEARBIT(raw, 70);
	SETBIT(raw, 71);

	return uuid_string_from_bits(raw);
}


/// Open file for reading regardless of OS
/// NOTE: Disabled Windows variant as this seemed to break parsing files with non-ASCII characters
/// (the opposite of what this is supposed to do...)
FILE * flex_fopen(const char * fname) {
	FILE * in = NULL;

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	in = fopen(fname, "rb");
	// int wchars_num = MultiByteToWideChar(CP_UTF8, 0, fname, -1, NULL, 0);
	// wchar_t * wstr = malloc(sizeof(wchar_t) * (wchars_num + 1));
	// MultiByteToWideChar(CP_UTF8, 0, fname, -1, wstr, wchars_num);

	// in = _wfopen(wstr, L"rb");

	// free(wstr);
#else
	in = fopen(fname, "r");
#endif

	return in;
}


/// Cross-platform dirname()
char * mmd_dirname(const char * path) {
	struct stat status;

	// We already have a directory
	if (stat(path, &status) == 0 && (status.st_mode & S_IFDIR)) {
		return my_strdup(path);
	}


#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	char * dir = malloc(sizeof(char) * _MAX_DIR);

	_splitpath_s(path, NULL, 0, dir, _MAX_DIR, NULL, 0, NULL, 0);

	return dir;
#else
	char * dir = my_strdup(dirname((char *) path));
	return dir;
#endif
}


/// Check text for a specified char
int text_contains_char(const char * text, size_t len, char target) {
	const char * stop = text + len;

	while (*text != '\0' && text < stop) {
		if (*text == target) {
			return 1;
		}

		text++;
	}

	return 0;
}
