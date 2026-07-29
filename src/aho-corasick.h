/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file aho-corasick.h

	@brief The Aho-Corasick algorithm allows searching text for multiple strings
	simultaneously in linear time.  This implementation allows returning all matches,
	or limiting to only the leftmost match or the leftmost longest match.


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


#ifndef AHO_CORASICK_H
#define AHO_CORASICK_H


/// Search results are returned in the form of a
/// linked list of matches
struct match {
	size_t			start;
	size_t			len;
	unsigned char	type;

	struct match *			next;
	struct match *			prev;
};

typedef struct match match;


// Opaque AC trie structure
typedef struct ac ac;


/// Change the matching behavior
enum aho_corasick_options {
	AC_LEFTMOST		= 1 << 0,		//!< Prevent overlapping matches
	AC_LONGEST		= 1 << 1,		//!< Return only longest match, requires AC_LEFTMOST also
};



/// Create new search trie
ac * ac_new(int startingSize);


/// Free existing trie
void ac_free(ac * a);


/// Add search term to the trie
bool ac_insert(ac * a, const char * term, unsigned char type);


/// Prepare the trie for use
void ac_prepare(ac * a, int options);


/// Perform search - returned link list must be freed
match * ac_search(ac * a, int options, const unsigned char * source, size_t start, size_t len);


/// Monitor one character at a time for matches
size_t ac_step(size_t s, ac * a, int options, unsigned char c, size_t * len, unsigned char * type);


/// Free linked list of matches
void match_free(match * m);


/// Visualize the trie for debugging and analysis
void ac_to_graphviz(ac * a, FILE * out);


#endif
