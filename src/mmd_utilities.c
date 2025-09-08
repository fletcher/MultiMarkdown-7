/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_utilities.c

	@brief


	@author	Fletcher T. Penney
	@bug

**/

/*

	MIT License

	Copyright (c) 2024-2025 Fletcher T. Penney

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


#include <stdlib.h>
#include <time.h>


#include "libMultiMarkdown.h"

#include "mmd_utilities.h"


int table_has_caption(mmd_node * t) {
	if (t && t->next && t->next->type == BLOCK_PARA) {
		t = t->next->content;

		if (t && t->type == TOKEN_PAIR_BRACKET) {
			t = t->next->next;

			if (t && t->type == TOKEN_PAIR_BRACKET) {
				t = t->next->next;
			}

			if (t == NULL) {
				return 1;
			}

			if (t && ((t->type == TOKEN_NL) || (t->type == TOKEN_LINEBREAK))) {
				return 1;
			}
		}
	}

	return 0;
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

