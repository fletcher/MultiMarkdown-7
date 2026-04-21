/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file aho-corasick.c

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


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aho-corasick.h"
#include "text_buffer.h"


#ifdef TEST
	#include "CuTest.h"
#endif

#define kTrieStartingSize 256

#define F(i,n) for(int i= 0;i<n;i++)


/// Node used to build Aho-Corasick search trie
typedef struct {
	unsigned char	c;
	unsigned char	type;
	int				len;

	size_t			child[256];
	size_t			ac_fail;
} trie_node;


/// Aho-Corasick search trie
struct ac {
	size_t			size;
	size_t			capacity;

	trie_node *		node;
};


/// Create new search trie
ac * ac_new(int startingSize) {
	ac * a = malloc(sizeof(ac));

	if (a) {
		if (startingSize <= 0) {
			startingSize = kTrieStartingSize;
		}

		a->node = calloc(startingSize, sizeof(trie_node));

		if (!a->node) {
			free(a);
			return NULL;
		}

		// All tries have a root node
		a->size = 1;
		a->capacity = startingSize;
	}

	return a;
}


/// Free existing trie
void ac_free(ac * a) {
	if (a) {
		free(a->node);
		free(a);
	}
}


static bool ac_insert_node(ac * a, size_t s, const unsigned char * term, unsigned char type, int depth) {
	if (a) {
		// Get node for state s
		trie_node * n = &a->node[s];

		size_t i;

		if (term[0] == '\0') {
			// We've hit the end of the key -- success
			n->type = type;
			n->len = depth;

			return true;
		}

		if (n->child[term[0]] != 0) {
			// This character is already in the trie, advance forward
			return ac_insert_node(a, n->child[term[0]], term + 1, type, ++depth);
		}

		// Ensure capacity
		if (a->size == a->capacity) {
			a->capacity *= 2;
			a->node = realloc(a->node, a->capacity * sizeof(trie_node));

			// Set n to new location
			n = &(a->node[s]);
		}

		// Current node points to next node
		i = a->size;
		n->child[term[0]] = i;

		// Initialize new node to 0
		n = &a->node[i];
		memset(n, 0, sizeof(trie_node));

		// Set char for new node
		n->c = term[0];

		// Increment size
		a->size++;

		// Advance forward
		return ac_insert_node(a, i, term + 1, type, ++depth);
	}

	return false;
}


/// Add search term to the trie
bool ac_insert(ac * a, const char * term, unsigned char type) {
	if (a && term && term[0] != '\0') {
		return ac_insert_node(a, 0, (const unsigned char *) term, type, 0);
	}

	return false;
}


#ifdef TEST
void Test_ac_insert_node(CuTest * tc) {
	ac * a = ac_new(0);

	CuAssertIntEquals(tc, kTrieStartingSize, a->capacity);
	CuAssertIntEquals(tc, 1, a->size);

	ac_insert(a, "foo", 42);

	trie_node * n = &a->node[0];
	CuAssertIntEquals(tc, 0, n->type);
	CuAssertIntEquals(tc, 1, n->child['f']);
	CuAssertIntEquals(tc, '\0', n->c);

	n = &a->node[1];
	CuAssertIntEquals(tc, 0, n->type);
	CuAssertIntEquals(tc, 2, n->child['o']);
	CuAssertIntEquals(tc, 'f', n->c);

	n = &a->node[2];
	CuAssertIntEquals(tc, 0, n->type);
	CuAssertIntEquals(tc, 3, n->child['o']);
	CuAssertIntEquals(tc, 'o', n->c);

	n = &a->node[3];
	CuAssertIntEquals(tc, 42, n->type);
	CuAssertIntEquals(tc, 3, n->len);
	CuAssertIntEquals(tc, 'o', n->c);

	ac_free(a);
}

#endif


static size_t trie_node_search(ac * a, size_t s, const unsigned char * query) {
	if (query[0] == '\0') {
		// Found matching state
		return s;
	}

	if (a->node[s].child[query[0]] == 0) {
		// Failed to match
		return -1;
	}

	// Partial match, keep going
	return trie_node_search(a, a->node[s].child[query[0]], query + 1);
}


static size_t trie_search(ac * a, const char * query) {
	if (a && query) {
		return trie_node_search(a, 0, (const unsigned char *) query);
	}

	return 0;
}


#ifdef TEST
static unsigned char trie_search_match_type(ac * a, const char * query) {
	size_t s = trie_search(a, query);

	if (s == (size_t) -1) {
		return -1;
	}

	return a->node[s].type;
}


void Test_trie_search(CuTest * tc) {
	ac * a = ac_new(0);

	ac_insert(a, "foo", 42);
	ac_insert(a, "bar", 41);
	ac_insert(a, "food", 40);

	CuAssertIntEquals(tc, 3, trie_search(a, "foo"));
	CuAssertIntEquals(tc, 42, trie_search_match_type(a, "foo"));

	CuAssertIntEquals(tc, 6, trie_search(a, "bar"));
	CuAssertIntEquals(tc, 41, trie_search_match_type(a, "bar"));

	CuAssertIntEquals(tc, 7, trie_search(a, "food"));
	CuAssertIntEquals(tc, 40, trie_search_match_type(a, "food"));

	CuAssertIntEquals(tc, -1, trie_search(a, "foot"));
	CuAssertIntEquals(tc, (unsigned char) - 1, trie_search_match_type(a, "foot"));
}

#endif


static void trie_node_prepare(ac * a, int options, size_t s, char * buffer, int depth, char * last_match) {
	buffer[depth] = '\0';
	buffer[depth + 1] = '\0';

	// Current node
	trie_node * n = &(a->node[s]);

	//char * suffix = buffer;
	char * suffix = last_match;

	// Longest match seen so far
	suffix += 1;

	// Find valid suffixes for failure path
	switch (options) {
		case AC_LEFTMOST:
		case AC_LEFTMOST | AC_LONGEST:

			// Don't fail on a match
			if (n->type == 0) {
				while ((suffix[0] != '\0') && (n->ac_fail == 0)) {
					n->ac_fail = trie_search(a, suffix);

					if (n->ac_fail == (size_t) -1) {
						n->ac_fail = 0;
					}

					if (n->ac_fail == s) {
						// Something went wrong
						fprintf(stderr, "Recursive trie fallback detected at state %zu('%c') - suffix:'%s'!\n", s, n->c, suffix);
						n->ac_fail = 0;
					}

					suffix++;
				}
			} else {
				// This is a match
				last_match = buffer + depth;
			}

			break;

		default:
			while ((suffix[0] != '\0') && (n->ac_fail == 0)) {
				n->ac_fail = trie_search(a, suffix);

				if (n->ac_fail == (size_t) -1) {
					n->ac_fail = 0;
				}

				if (n->ac_fail == s) {
					// Something went wrong
					fprintf(stderr, "Recursive trie fallback detected at state %zu('%c') - suffix:'%s'!\n", s, n->c, suffix);
					n->ac_fail = 0;
				}

				suffix++;
			}

			break;
	}

	// Prepare child nodes
	F(i, 256) {
		if ((n->child[i] != 0) && (n->child[i] != s)) {
			buffer[depth] = i;

			trie_node_prepare(a, options, n->child[i], buffer, depth + 1, last_match);
		}
	}
}


/// Prepare the trie for use
void ac_prepare(ac * a, int options) {
	if (a) {
		// Clear old pointers
		F(i, (int) a->size) {
			a->node[i].ac_fail = 0;
		}

		// Create a buffer
		char * buffer = malloc(sizeof(char) * (a->capacity + 1));

		trie_node_prepare(a, options, 0, buffer, 0, buffer);

		free(buffer);
	}
}


#ifdef TEST
void Test_ac_prepare(CuTest * tc) {
	ac * a = ac_new(0);

	ac_insert(a, "a", 1);
	ac_insert(a, "aa", 2);
	ac_insert(a, "aaa", 3);
	ac_insert(a, "aaaa", 4);

	ac_prepare(a, 0);

	CuAssertIntEquals(tc, 5, a->size);

	ac_free(a);
}

#endif


static match * match_new(size_t start, size_t len, unsigned char type) {
	match * m = calloc(1, sizeof(match));

	if (m) {
		m->start = start;
		m->len = len;
		m->type = type;
	}

	return m;
}


static match * match_append(match * last, size_t start, size_t len, unsigned char type) {
	if (last) {
		last->next = match_new(start, len, type);
		last->next->prev = last;
		return last->next;
	} else {
		return match_new(start, len, type);
	}
}


void match_free(match * m) {
	match * next;

	while (m) {
		next = m->next;
		free(m);
		m = next;
	}
}


/// Perform search
match * ac_search(ac * a, int options, const unsigned char * source, size_t start, size_t len) {
	// Store results in a linked list
	match * result = NULL;
	match * m = result;

	// Keep track of state
	size_t s = 0;
	size_t temp_s;

	size_t counter = start;
	size_t stop = start + len;

	while ((counter < stop) && (source[counter] != '\0')) {
		// Check for path that allows us to match next character
		while (s && a->node[s].child[source[counter]] == 0) {
			s = a->node[s].ac_fail;
		}

		// Advance state for the next character
		s = a->node[s].child[source[counter++]];

		// Check for partial matches
		temp_s = s;

		while (temp_s) {
			if (a->node[temp_s].type) {
				// This is a match
				if (m) {
					if (options & AC_LONGEST) {
						// Is this longer than the current match?
						if (m->start == counter - a->node[temp_s].len) {
							// Update existing match instead of appending.
							m->len = a->node[temp_s].len;
							m->type = a->node[temp_s].type;
						} else {
							m = match_append(m, counter - a->node[temp_s].len, a->node[temp_s].len, a->node[temp_s].type);
						}
					} else {
						m = match_append(m, counter - a->node[temp_s].len, a->node[temp_s].len, a->node[temp_s].type);
					}
				} else {
					result = match_new(counter - a->node[temp_s].len, a->node[temp_s].len, a->node[temp_s].type);
					m = result;
				}
			}

			temp_s = a->node[temp_s].ac_fail;
		}
	}

	return result;
}


#ifdef TEST

static void match_set_buffer(match * m, const char * text, text_buffer * buffer) {
	while (m) {
		text_buffer_append_printf(buffer, "%.*s\n", (int)m->len, &text[m->start]);

		m = m->next;
	}
}


int options[] = {
	0,
	AC_LEFTMOST,
	AC_LEFTMOST | AC_LONGEST
};


char * haystack[] = {
	"footbally",
	"ufootbally",
	"footbal",
	"föot"
};


char * results[3][4] = {
	{
		"foot\notb\nfootball\nball\nally\n",
		"ufo\nfoot\notb\nfootball\nball\nally\n",
		"foot\notb\n",
		"föot\n"
	},
	{
		"foot\nfootball\n",
		"ufo\notb\nally\n",
		"foot\n",
		"föot\n"
	},
	{
		"football\n",
		"ufo\notb\nally\n",
		"foot\n",
		"föot\n"
	}
};

void Test_ac_search(CuTest * tc) {
	ac * a = ac_new(0);

	ac_insert(a, "foo", 42);
	ac_insert(a, "bar", 41);
	ac_insert(a, "food", 40);
	ac_insert(a, "oo", 39);

	ac_prepare(a, 0);

	match * m = ac_search(a, 0, (const unsigned char *) "This is a bar that serves food.", 0, 31);

	CuAssertPtrNotNull(tc, m);
	CuAssertIntEquals(tc, 41, m->type);
	CuAssertIntEquals(tc, 10, m->start);
	CuAssertIntEquals(tc, 3, m->len);

	CuAssertPtrNotNull(tc, m->next);
	CuAssertIntEquals(tc, 42, m->next->type);
	CuAssertIntEquals(tc, 26, m->next->start);
	CuAssertIntEquals(tc, 3, m->next->len);

	CuAssertPtrNotNull(tc, m->next->next);
	CuAssertIntEquals(tc, 39, m->next->next->type);
	CuAssertIntEquals(tc, 27, m->next->next->start);
	CuAssertIntEquals(tc, 2, m->next->next->len);

	CuAssertPtrNotNull(tc, m->next->next->next);
	CuAssertIntEquals(tc, 40, m->next->next->next->type);
	CuAssertIntEquals(tc, 26, m->next->next->next->start);
	CuAssertIntEquals(tc, 4, m->next->next->next->len);

	CuAssertPtrEquals(tc, NULL, m->next->next->next->next);

	ac_free(a);
	match_free(m);

	text_buffer * buf;

	// Create reusable AC trie
	a = ac_new(0);
	ac_insert(a, "foot", 42);
	ac_insert(a, "football", 41);
	ac_insert(a, "ball", 40);
	ac_insert(a, "ally", 39);
	ac_insert(a, "ufo", 38);
	ac_insert(a, "otb", 37);
	ac_insert(a, "föot", 69);

	F(i, (int) (sizeof(options) / sizeof(options[0]))) {
		// Prepare AC trie with new options
		ac_prepare(a, options[i]);

		// ac_to_graphviz(a, stdout);

		// Test with multiple haystacks
		F(j, (int) (sizeof(haystack) / sizeof(haystack[0]))) {
			m = ac_search(a, options[i], (const unsigned char *) haystack[j], 0, strlen(haystack[j]));
			buf = text_buffer_new(0);

			match_set_buffer(m, haystack[j], buf);

			CuAssertStrEquals(tc, results[i][j], buf->text);

			match_free(m);
			text_buffer_free(buf, true);
		}
	}

	ac_free(a);
}

#endif


/// Monitor one character at a time for matches
size_t ac_step(size_t s, ac * a, int options, unsigned char c, size_t * len, unsigned char * type) {
	*len = -1;
	*type = '\0';

	// Check for path that allows us to match next character
	while (s && a->node[s].child[c] == 0) {
		s = a->node[s].ac_fail;
	}

	// Accept next character
	s = a->node[s].child[c];

	// Do we have a match
	size_t temp_s = s;

	while (temp_s) {
		if (a->node[temp_s].type) {
			// This is a match
			if (*len != (size_t) -1) {
				if (options & AC_LONGEST) {
					// Is this longer than the current match?
					if (*len == (size_t) a->node[temp_s].len) {
						// Update existing match
						*len = a->node[temp_s].len;
						*type = a->node[temp_s].type;
					} else {
						// Ignore this match
					}
				} else {
					// Ignore this match
				}
			} else {
				*len = a->node[temp_s].len;
				*type = a->node[temp_s].type;
			}
		}

		temp_s = a->node[temp_s].ac_fail;
	}

	return s;
}


#ifdef TEST

unsigned char step_result[3][6] = {
	{ 0, 0, 42, 0, 0, 44 },
	{ 0, 0, 42, 0, 0, 44 },
	{ 0, 0, 42, 0, 0, 44 }
};

void Test_ac_step(CuTest * tc) {
	ac * a = ac_new(0);

	ac_insert(a, "foo", 42);
	ac_insert(a, "bar", 43);
	ac_insert(a, "foobar", 44);

	char * haystack = "foobar";

	F(i, (int) (sizeof(options) / sizeof(options[0]))) {
		ac_prepare(a, options[i]);
		size_t s = 0;
		size_t len = 0;
		unsigned char type = 0;

		F(j, (int) sizeof(haystack)) {
			s = ac_step(s, a, options[i], (unsigned char) haystack[j], &len, &type);
			CuAssertIntEquals(tc, step_result[i][j], type);

			switch (type) {
				case 0:
					break;

				case 42:
					CuAssertIntEquals(tc, 3, (int) len);
					break;

				case 43:
					CuAssertIntEquals(tc, 3, (int) len);
					break;

				case 44:
					CuAssertIntEquals(tc, 6, (int) len);
					break;
			}
		}
	}

	ac_free(a);
}

#endif


static void trie_node_to_graphviz(ac * a, size_t s, FILE * out) {
	trie_node * n = &a->node[s];

	if (n->type) {
		// This is a matching node
		fprintf(out, "\"%zu\" [shape=doublecircle]\n", s);
	}

	F(i, 256) {
		if (n->child[i]) {
			fprintf(out, "\"%zu\" -> \"%zu\" [label=\"%c\"]\n", s, n->child[i], (char)i);
		}
	}

	if (n->ac_fail) {
		fprintf(out, "\"%zu\" -> \"%zu\" [label=\"fail\"]\n", s, n->ac_fail);
	}
}


/// Visualize the trie for debugging and analysis
void ac_to_graphviz(ac * a, FILE * out) {
	fprintf(out, "digraph dfa {\n");

	F(i, (int) a->size) {
		trie_node_to_graphviz(a, i, out);
	}

	fprintf(out, "}\n");
}
