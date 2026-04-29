/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_span_parser.c

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


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "libMultiMarkdown7.h"

#include "char.h"
#include "mmd_node.h"
#include "mmd_scanner.h"
#include "text_buffer.h"
#include "read_ctx.h"
#include "stack.h"
#include "mmd_span_parser.h"
#include "mmd_token_scanner.h"
#include "mmd_tokenizer.h"


static PairRule pairings1[OBJECT_REPLACEMENT_CHARACTER] = {

	[TOKEN_CM_ADD_CLOSE] =	{ 0, TOKEN_CM_ADD_OPEN,	TOKEN_PAIR_CM_ADD	},
	[TOKEN_CM_DEL_CLOSE] =	{ 0, TOKEN_CM_DEL_OPEN,	TOKEN_PAIR_CM_DEL	},
	[TOKEN_CM_COM_CLOSE] =	{ 0, TOKEN_CM_COM_OPEN,	TOKEN_PAIR_CM_COM	},
	[TOKEN_CM_HI_CLOSE] =	{ 0, TOKEN_CM_HI_OPEN,	TOKEN_PAIR_CM_HI	},
	[TOKEN_CM_SUB_DIV] =	{ 0, TOKEN_CM_SUB_OPEN,	TOKEN_PAIR_CM_SUB_DEL	},
	[TOKEN_CM_SUB_CLOSE] =	{ 0, TOKEN_CM_SUB_DIV,	TOKEN_PAIR_CM_SUB_ADD	},
};


static PairRule pairings3[OBJECT_REPLACEMENT_CHARACTER] = {

	[TOKEN_ANGLE_RIGHT] =	{ 0, TOKEN_ANGLE_LEFT,		TOKEN_PAIR_ANGLE	},
	[TOKEN_BRACE_RIGHT] =	{ 0, TOKEN_BRACE_LEFT,		TOKEN_PAIR_BRACE	},
	[TOKEN_BRACKET_RIGHT] =	{ PAIR_HAS_SUBTYPES, TOKEN_BRACKET_LEFT,	TOKEN_PAIR_BRACKET	},
	[TOKEN_PAREN_RIGHT] =	{ 0, TOKEN_PAREN_LEFT,		TOKEN_PAIR_PAREN	},

	[TOKEN_BACKTICK] =		{ PAIR_MATCH_LENGTH,	TOKEN_BACKTICK, TOKEN_PAIR_BACKTICK	},

	[TOKEN_MATH_PAREN_CLOSE] =		{ 0,	TOKEN_MATH_PAREN_OPEN,	TOKEN_PAIR_MATH_PAREN },
	[TOKEN_MATH_BRACKET_CLOSE] =	{ 0,	TOKEN_MATH_BRACKET_OPEN,	TOKEN_PAIR_MATH_BRACKET },

	[TOKEN_MATH_DOLLAR_SINGLE] =	{ PAIR_OPEN_IF_WS_LE_LEFT | PAIR_OPEN_NO_WS_LE_RIGHT | PAIR_CLOSE_IF_WS_LE_RIGHT | PAIR_CLOSE_IF_PUNCT_RIGHT | PAIR_CLOSE_NO_WS_LE_LEFT,	TOKEN_MATH_DOLLAR_SINGLE,	TOKEN_PAIR_MATH_DOLLAR_SINGLE },
	[TOKEN_MATH_DOLLAR_DOUBLE] =	{ PAIR_OPEN_IF_WS_LE_LEFT | PAIR_OPEN_NO_WS_LE_RIGHT | PAIR_CLOSE_IF_WS_LE_RIGHT | PAIR_CLOSE_IF_PUNCT_RIGHT | PAIR_CLOSE_NO_WS_LE_LEFT,	TOKEN_MATH_DOLLAR_DOUBLE,	TOKEN_PAIR_MATH_DOLLAR_DOUBLE },
};


static PairRule pairings4[OBJECT_REPLACEMENT_CHARACTER] = {

	[TOKEN_QUOTE_SINGLE] = 	{ PAIR_OPEN_NO_WS_LE_RIGHT | PAIR_OPEN_NO_ALPHA_NUM_LEFT | PAIR_CLOSE_NO_WS_LE_LEFT,	TOKEN_QUOTE_SINGLE, TOKEN_PAIR_QUOTE_SINGLE	},
	[TOKEN_QUOTE_DOUBLE] = 	{ PAIR_OPEN_NO_WS_LE_RIGHT | PAIR_OPEN_NO_ALPHA_NUM_LEFT | PAIR_CLOSE_NO_WS_LE_LEFT,	TOKEN_QUOTE_DOUBLE, TOKEN_PAIR_QUOTE_DOUBLE	},
	[TOKEN_QUOTE_DOUBLE_ALT] = 	{ PAIR_OPEN_NO_WS_LE_RIGHT | PAIR_CLOSE_NO_WS_LE_LEFT | PAIR_MATCH_LENGTH,	TOKEN_BACKTICK, TOKEN_PAIR_QUOTE_DOUBLE	},

	[TOKEN_STAR] = 			{ PAIR_OPEN_NO_WS_LE_RIGHT | PAIR_CLOSE_NO_WS_LE_LEFT | PAIR_SKIP_IDENTICAL_TOKENS | PAIR_LIMIT_MULTIPLE_3,	TOKEN_STAR,	TOKEN_PAIR_STAR	},
	[TOKEN_UL] = 			{ PAIR_OPEN_NO_WS_LE_RIGHT | PAIR_CLOSE_NO_WS_LE_LEFT | PAIR_SKIP_IDENTICAL_TOKENS | PAIR_NO_MATCH_INTRAWORD,	TOKEN_UL,	TOKEN_PAIR_UL	},

	[TOKEN_SUPERSCRIPT] =	{ PAIR_OPEN_NO_WS_LE_RIGHT | PAIR_CLOSE_NO_WS_LE_LEFT | PAIR_NON_CONSECUTIVE, TOKEN_SUPERSCRIPT, TOKEN_PAIR_SUPERSCRIPT },
	[TOKEN_SUBSCRIPT] =		{ PAIR_OPEN_NO_WS_LE_RIGHT | PAIR_CLOSE_NO_WS_LE_LEFT | PAIR_NON_CONSECUTIVE, TOKEN_SUBSCRIPT, TOKEN_PAIR_SUBSCRIPT },
};


static int delimiter_len(const char * text, size_t pos) {
	int l = 1;
	char c = text[pos];

	size_t curs = pos;

	while (text[++curs] == c) {
		l++;
	}

	curs = pos;

	while (curs && (text[--curs] == c)) {
		l++;
	}

	return l;
}


// Ensure this is valid closer
static int token_can_close(mmd_node * n, const char * text, PairRule pairings[]) {
	size_t offset = n->start;

	char c = text[offset];

	// Check left
	if (offset) {
		offset--;

		if (pairings[n->type].conditions & PAIR_SKIP_IDENTICAL_TOKENS) {
			while (offset && text[offset] == c) {
				offset--;
			}

			if (text[offset] == c) {
				return 0;
			}
		}

		// Must not fail these
		if (pairings[n->type].conditions & PAIR_CLOSE_NO_WS_LE_LEFT) {
			if (char_is_whitespace_or_line_ending(text[offset])) {
				return 0;
			}
		}

		if (pairings[n->type].conditions & PAIR_CLOSE_NO_PUNCT_LEFT) {
			if (char_is_punctuation(text[offset])) {
				return 0;
			}
		}

		if (pairings[n->type].conditions & PAIR_CLOSE_NO_ALPHA_NUM_LEFT) {
			if (char_is_alphanumeric(text[offset])) {
				return 0;
			}
		}

		// Must meet one of these
		if (pairings[n->type].conditions & (PAIR_CLOSE_IF_WS_LE_LEFT | PAIR_CLOSE_IF_PUNCT_LEFT)) {
			int meet_req = 0;

			if (pairings[n->type].conditions & PAIR_CLOSE_IF_WS_LE_LEFT) {
				if (char_is_whitespace_or_line_ending(text[offset])) {
					meet_req = 1;
				}
			}

			if (pairings[n->type].conditions & PAIR_CLOSE_IF_PUNCT_LEFT) {
				if (char_is_punctuation(text[offset])) {
					meet_req = 1;
				}
			}

			if (!meet_req) {
				return 0;
			}
		}
	} else {
		// Can't close if we're first character
		return 0;
	}


	// Check right
	offset = n->start + n->len;

	if (pairings[n->type].conditions & PAIR_SKIP_IDENTICAL_TOKENS) {
		while (text[offset] == c) {
			offset++;
		}
	}

	// Must not fail these
	if (pairings[n->type].conditions & PAIR_CLOSE_NO_WS_LE_RIGHT) {
		if (char_is_whitespace_or_line_ending(text[offset])) {
			return 0;
		}
	}

	if (pairings[n->type].conditions & PAIR_CLOSE_NO_PUNCT_RIGHT) {
		if (char_is_punctuation(text[offset])) {
			return 0;
		}
	}

	if (pairings[n->type].conditions & PAIR_CLOSE_NO_ALPHA_NUM_RIGHT) {
		if (char_is_alphanumeric(text[offset])) {
			return 0;
		}
	}

	// Must meet one of these
	if (pairings[n->type].conditions & (PAIR_CLOSE_IF_WS_LE_RIGHT | PAIR_CLOSE_IF_PUNCT_RIGHT)) {
		int meet_req = 0;

		if (pairings[n->type].conditions & PAIR_CLOSE_IF_WS_LE_RIGHT) {
			if (char_is_whitespace_or_line_ending(text[offset])) {
				meet_req = 1;
			}
		}

		if (pairings[n->type].conditions & PAIR_CLOSE_IF_PUNCT_RIGHT) {
			if (char_is_punctuation(text[offset])) {
				meet_req = 1;
			}
		}

		if (!meet_req) {
			return 0;
		}
	}


	// Are intraword matches allowed?  (e.g. `_` is not)
	if (pairings[n->type].conditions & PAIR_NO_MATCH_INTRAWORD) {
		offset = n->start;

		if (offset) {
			offset--;

			if (pairings[n->type].conditions & PAIR_SKIP_IDENTICAL_TOKENS) {
				while (text[offset] == text[n->start]) {
					offset--;
				}
			}

			if (!char_is_whitespace_or_line_ending(text[offset])) {
				// Punctuation not allowed to left
				// Word fragment to left

				// Now check right
				offset = n->start + n->len;

				if (pairings[n->type].conditions & PAIR_SKIP_IDENTICAL_TOKENS) {
					while (text[offset] == text[n->start]) {
						offset++;
					}
				}

				if (!char_is_whitespace_or_line_ending_or_punctuation(text[offset])) {
					// Punctuation allowed to right
					// We are intraword
					return 0;
				}
			}
		}
	}

	return 1;
}


static int token_can_open(mmd_node * n, const char * text, PairRule pairings[]);

static int delimiter_valid_multiple_3(const char * text, mmd_node * o, mmd_node * c, PairRule pairings[]) {
	int len_o = delimiter_len(text, o->start);
	int len_c = delimiter_len(text, c->start);

	// If the sum of the lengths is not a multiple of 3, it's valid
	if ((len_o + len_c) % 3) {
		return 1;
	}

	// If both lengths are a multiple of 3, it's valid
	if ((len_o % 3) == (len_c % 3) && (len_o % 3) == 0) {
		return 1;
	}

	// If neither is "ambidextrous", then it's valid
	if (!token_can_close(o, text, pairings) && !token_can_open(c, text, pairings)) {
		return 1;
	}

	return 0;
}


static mmd_node * token_closes(mmd_node_pool * p, mmd_node * n, mmd_node * prev, PairRule pairings[], const char * text, stack * s, size_t stack_start, unsigned int openers[255], uint32_t options) {
	// First, make sure there is an opener before we see about closing
	if (!openers[pairings[n->type].mate_type]) {
		return NULL;
	}

	if (!token_can_close(n, text, pairings)) {
		return NULL;
	}

	// We're a valid closer, so look for the match
	mmd_node * o;

	unsigned target_type = pairings[n->type].mate_type;
	int match_len = (pairings[n->type].conditions  & PAIR_MATCH_LENGTH);

	if (match_len) {
		// We must match length, so have to scan before popping from stack
		size_t i = s->size;
		mmd_node * match = NULL;

		while (i > stack_start) {
			o = stack_peek_index(s, i - 1);

			if ((o->type == target_type) && (o->len == n->len)) {
				match = o;
				break;
			}

			i--;
		}

		if (match == NULL) {
			// No match with correct length
			return NULL;
		}
	}

	int limit_multiple_3 = (pairings[n->type].conditions & PAIR_LIMIT_MULTIPLE_3);

	if (limit_multiple_3) {
		// Intraword strong/emph -- limits to match CommonMark behavior
		// If opener or closer is "ambidextrous", then
		// sum of delimiter runs must not be multiple of 3 unless both lengths are multiples of 3
		size_t i = s->size;

		mmd_node * match = NULL;

		while (i > stack_start) {
			o = stack_peek_index(s, i - 1);

			if ((o->type == target_type) && delimiter_valid_multiple_3(text, o, n, pairings)) {
				match = o;
				break;
			}

			i--;
		}

		if (match == NULL) {
			return NULL;
		}
	}

	int non_consecutive = (pairings[n->type].conditions & PAIR_NON_CONSECUTIVE);

	// Find opener and pair off
	while (s->size > stack_start && (o = stack_pop(s))) {
		openers[o->type]--;

		if ((o->type == target_type) && (!match_len || o->len == n->len) &&
				(!limit_multiple_3 || delimiter_valid_multiple_3(text, o, n, pairings)) &&
				(!non_consecutive || o->next != n)
		   ) {
			// We have the match
			if (o->next != n) {
				o->child = o->next;
				o->child->tail = prev;
			}

			char pair_type = (options & MMD_OPTION_COMPATIBILITY) ? 'x' : text[o->start + 1];

			if (pairings[n->type].conditions & PAIR_HAS_SUBTYPES) {
				// Different openers lead to different closing pair types
				switch (n->type) {
					case TOKEN_BRACKET_RIGHT:

						// So far, this is the only time this is used
						switch (pair_type) {
							case '>':
								// Abbreviation
								o->type = TOKEN_PAIR_BRACKET_ABBREVIATION;
								mmd_node_split(p, o->child, 1);
								o->child->type = TOKEN_ABBREVIATION_MARKER;

								if (prev == o->child && o->child->next != n) {
									prev = o->child->next;
								}

								break;

							case '#':
								// Citation
								o->type = TOKEN_PAIR_BRACKET_CITATION;
								mmd_node_split(p, o->child, 1);

								// This is no longer a TOKEN_TAG
								if (o->child->next && o->child->next->type == TOKEN_TAG) {
									o->child->next->type = TOKEN_TEXT;
								}

								o->child->type = TOKEN_CITATION_MARKER;

								if (prev == o->child && o->child->next != n) {
									prev = o->child->next;
								}

								break;

							case '^':
								// Footnote
								o->type = TOKEN_PAIR_BRACKET_FOOTNOTE;
								mmd_node_split(p, o->child, 1);
								o->child->type = TOKEN_FOOTNOTE_MARKER;

								if (prev == o->child && o->child->next != n) {
									prev = o->child->next;
								}

								break;

							case '?':
								// Glossary
								o->type = TOKEN_PAIR_BRACKET_GLOSSARY;
								mmd_node_split(p, o->child, 1);
								o->child->type = TOKEN_GLOSSARY_MARKER;

								if (prev == o->child && o->child->next != n) {
									prev = o->child->next;
								}

								break;

							case '%':
								// Variable
								o->type = TOKEN_PAIR_BRACKET_VARIABLE;
								mmd_node_split(p, o->child, 1);
								o->child->type = TOKEN_VARIABLE_MARKER;

								if (prev == o->child && o->child->next != n) {
									prev = o->child->next;
								}

								break;

							default:
								if (text[o->start] == '!') {
									o->type = TOKEN_PAIR_BRACKET_IMAGE;
								} else {
									o->type = TOKEN_PAIR_BRACKET;
								}

								break;
						}
				}
			} else {
				o->type = pairings[n->type].pairing_type;
			}

			o->next = n;

			if (prev && o != prev) {
				prev->next = NULL;
			}

			return o;
		}
	}

	return NULL;
}


static int token_can_open(mmd_node * n, const char * text, PairRule pairings[]) {
	// Ensure this is valid opener
	size_t offset = n->start;

	char c = text[offset];

	// Check left
	if (offset) {
		offset--;

		if (pairings[n->type].conditions & PAIR_SKIP_IDENTICAL_TOKENS) {
			while (offset && text[offset] == c) {
				offset--;
			}
		}

		// Must not fail these
		if (pairings[n->type].conditions & PAIR_OPEN_NO_WS_LE_LEFT) {
			if (char_is_whitespace_or_line_ending(text[offset])) {
				return 0;
			}
		}

		if (pairings[n->type].conditions & PAIR_OPEN_NO_PUNCT_LEFT) {
			if (char_is_punctuation(text[offset])) {
				return 0;
			}
		}

		if (pairings[n->type].conditions & PAIR_OPEN_NO_ALPHA_NUM_LEFT) {
			if (char_is_alphanumeric(text[offset])) {
				return 0;
			}
		}

		// Must meet one of these
		if (pairings[n->type].conditions & (PAIR_OPEN_IF_WS_LE_LEFT | PAIR_OPEN_IF_PUNCT_LEFT)) {
			int meet_req = 0;

			if (pairings[n->type].conditions & PAIR_OPEN_IF_WS_LE_LEFT) {
				if (char_is_whitespace_or_line_ending(text[offset])) {
					meet_req = 1;
				}
			}

			if (pairings[n->type].conditions & PAIR_OPEN_IF_PUNCT_LEFT) {
				if (char_is_punctuation(text[offset])) {
					meet_req = 1;
				}
			}

			if (!meet_req) {
				return 0;
			}
		}
	}


	// Check right
	offset = n->start + n->len;

	if (pairings[n->type].conditions & PAIR_SKIP_IDENTICAL_TOKENS) {
		while (text[offset] == c) {
			offset++;
		}
	}

	// Must not fail these
	if (pairings[n->type].conditions & PAIR_OPEN_NO_WS_LE_RIGHT) {
		if (char_is_whitespace_or_line_ending(text[offset])) {
			return 0;
		}
	}

	if (pairings[n->type].conditions & PAIR_OPEN_NO_PUNCT_RIGHT) {
		if (char_is_punctuation(text[offset])) {
			return 0;
		}
	}

	if (pairings[n->type].conditions & PAIR_OPEN_NO_ALPHA_NUM_RIGHT) {
		if (char_is_alphanumeric(text[offset])) {
			return 0;
		}
	}

	// Must meet one of these
	if (pairings[n->type].conditions & (PAIR_OPEN_IF_WS_LE_RIGHT | PAIR_OPEN_IF_PUNCT_RIGHT)) {
		int meet_req = 0;

		if (pairings[n->type].conditions & PAIR_OPEN_IF_WS_LE_RIGHT) {
			if (char_is_whitespace_or_line_ending(text[offset])) {
				meet_req = 1;
			}
		}

		if (pairings[n->type].conditions & PAIR_OPEN_IF_PUNCT_RIGHT) {
			if (char_is_punctuation(text[offset])) {
				meet_req = 1;
			}
		}

		if (!meet_req) {
			return 0;
		}
	}


	// Are intraword matches allowed?  (e.g. `_` is not)
	if (pairings[n->type].conditions & PAIR_NO_MATCH_INTRAWORD) {
		offset = n->start;

		if (offset) {
			offset--;

			if (pairings[n->type].conditions & PAIR_SKIP_IDENTICAL_TOKENS) {
				while (offset && (text[offset] == text[n->start])) {
					offset--;
				}
			}

			if (!char_is_whitespace_or_line_ending_or_punctuation(text[offset])) {
				// Punctuation allowed to left
				// Word fragment to left

				// Now check right
				offset = n->start + n->len;

				if (pairings[n->type].conditions & PAIR_SKIP_IDENTICAL_TOKENS) {
					while (text[offset] == text[n->start]) {
						offset++;
					}
				}

				if (!char_is_whitespace_or_line_ending(text[offset])) {
					// Punctuation not allowed to right
					// We are intraword
					return 0;
				}
			}
		}
	}

	return 1;
}


static int token_opens(mmd_node * n, PairRule pairings[], const char * text, stack * s, unsigned int openers[255]) {
	if (!token_can_open(n, text, pairings)) {
		return 0;
	}

	// We're a valid opener
	stack_push(s, n);
	openers[n->type]++;

	return 1;
}


/// Starting with first sequential opening token, assign strong/emph
static void pair_emphasis_tokens(mmd_node * n) {
	if (n == NULL) {
		return;
	}

	if ((n->type == TOKEN_PAIR_STAR) || (n->type == TOKEN_PAIR_UL)) {
		if (n->child && n->child->type == n->type) {
			if (n->next->type == n->child->tail->type) {
				n->type = TOKEN_PAIR_STRONG;

				if (n->child->child) {
					pair_emphasis_tokens(n->child->child);
					// Mark interior token pair as used variant
				}

				n->child->type++;
			} else {
				n->type = TOKEN_PAIR_EMPH;
				pair_emphasis_tokens(n->child);
			}
		} else {
			n->type = TOKEN_PAIR_EMPH;

			if (n->child) {
				pair_emphasis_tokens(n->child);
			}
		}
	}
}


void analyze_token_chain(mmd_node_pool * p, mmd_node * n, PairRule pairings[], const char * text, read_ctx * c, uint32_t options) {
	// Track count of available opening tokens
	unsigned int openers[255] = {0};

	// Track previous node
	mmd_node * opener, * temp, * prev = NULL;

	size_t offset;


	// Pair stack -- use a single stack for efficiency
	// Any tokens already on stack belong to a parent block
	stack * s = c->token_pair_stack;
	size_t stack_start = s->size;

	while (n) {
		offset = n->start;

		switch (n->type) {
			case TOKEN_CM_ADD_OPEN:
			case TOKEN_CM_DEL_OPEN:
			case TOKEN_CM_COM_OPEN:
			case TOKEN_CM_HI_OPEN:
			case TOKEN_CM_SUB_OPEN:
				if (!(options & MMD_OPTION_COMPATIBILITY)) {
					stack_push(s, n);
					openers[n->type]++;
				}

				break;

			case TOKEN_CM_SUB_DIV:
				if (!(options & MMD_OPTION_COMPATIBILITY)) {
					token_closes(p, n, prev, pairings, text, s, stack_start, openers, options);
					stack_push(s, n);
					openers[n->type]++;
				}

				break;

			case TOKEN_CM_SUB_CLOSE:
				if (!(options & MMD_OPTION_COMPATIBILITY)) {
					token_closes(p, n, prev, pairings, text, s, stack_start, openers, options);
				}

				break;

			case TOKEN_CM_ADD_CLOSE:
			case TOKEN_CM_DEL_CLOSE:
			case TOKEN_CM_COM_CLOSE:
			case TOKEN_CM_HI_CLOSE:
				if (!(options & MMD_OPTION_COMPATIBILITY)) {
					token_closes(p, n, prev, pairings, text, s, stack_start, openers, options);
				}

				break;

			case TOKEN_ANGLE_LEFT:
			case TOKEN_BRACE_LEFT:
			case TOKEN_BRACKET_LEFT:
			case TOKEN_PAREN_LEFT:
				stack_push(s, n);
				openers[n->type]++;
				break;

			case TOKEN_ANGLE_RIGHT:
			case TOKEN_BRACE_RIGHT:
			case TOKEN_BRACKET_RIGHT:
			case TOKEN_PAREN_RIGHT:
				token_closes(p, n, prev, pairings, text, s, stack_start, openers, options);
				break;

			case TOKEN_MATH_PAREN_OPEN:
			case TOKEN_MATH_BRACKET_OPEN:
				if (!(options & MMD_OPTION_COMPATIBILITY)) {
					stack_push(s, n);
					openers[n->type]++;
				}

				break;

			case TOKEN_MATH_PAREN_CLOSE:
			case TOKEN_MATH_BRACKET_CLOSE:
				if (!(options & MMD_OPTION_COMPATIBILITY)) {
					token_closes(p, n, prev, pairings, text, s, stack_start, openers, options);
				}

				break;

			case TOKEN_MATH_DOLLAR_SINGLE:
			case TOKEN_MATH_DOLLAR_DOUBLE:
				if (!(options & MMD_OPTION_COMPATIBILITY)) {
					// Skip if not being handled this pass
					if (!pairings[n->type].mate_type) {
						break;
					}

					if (!token_closes(p, n, prev, pairings, text, s, stack_start, openers, options)) {
						token_opens(n, pairings, text, s, openers);
					}
				}

				break;

			case TOKEN_BACKTICK:

				// Skip if not being handled this pass
				if (!(options & MMD_OPTION_COMPATIBILITY) && n->len == 2 && pairings[TOKEN_QUOTE_DOUBLE_ALT].mate_type) {
					// We could match ``foo''
					stack_push(s, n);
					openers[n->type]++;
				}

				if (!pairings[n->type].mate_type) {
					break;
				}

				if ((opener = token_closes(p, n, prev, pairings, text, s, stack_start, openers, options)) == NULL) {
					stack_push(s, n);
					openers[n->type]++;
				} else if (opener->child) {
					// Trim surrounding whitespace inside codespan
					while (char_is_whitespace_or_line_ending(text[opener->child->start])) {
						opener->child->start++;
						opener->child->len--;
					}

					while (opener->child->tail->len && char_is_whitespace_or_line_ending(text[opener->child->tail->start + opener->child->tail->len - 1])) {
						opener->child->tail->len--;
					}
				}

				break;

			case TOKEN_SUBSCRIPT:
			case TOKEN_SUPERSCRIPT:

				// Skip if not being handled this pass
				if (!pairings[n->type].mate_type) {
					break;
				}

				if (!token_closes(p, n, prev, pairings, text, s, stack_start, openers, options)) {
					// Can't open if whitespace to left
					if ((offset == 0) || char_is_whitespace_or_line_ending(text[offset - 1])) {
						// break;
					}

					// Or to right
					if (char_is_whitespace_or_line_ending(text[offset + 1])) {
						break;
					}

					// Requires a contiguous string of non-space characters to next token in order to create a pair
					const char * test = &text[n->start + 1];

					while ((*test != '\0') && !(char_is_whitespace_or_line_ending(*test))) {
						if (*test == text[n->start]) {
							token_opens(n, pairings, text, s, openers);
							break;
						}

						test++;
					}

					if (stack_peek(s) != n) {
						// Standalone?
						test = &text[n->start + 1];

						while ((*test != '\0') && !(char_is_whitespace_or_line_ending_or_punctuation(*test))) {
							test++;
						}

						if (test - &text[n->start] > 1) {
							offset = test - text;
							n->child = mmd_node_new(p, TOKEN_TEXT, n->start + 1, test - &text[n->start + 1]);
							n->type = (n->type == TOKEN_SUPERSCRIPT) ? TOKEN_PAIR_SUPERSCRIPT : TOKEN_PAIR_SUBSCRIPT;

							mmd_node * temp = n->next;

							while (temp && temp->start < offset) {
								if (temp->start + temp->len < offset) {
									// Entire token included
									temp->len = 0;
								} else {
									temp->len = temp->start + temp->len - offset;
									temp->start = offset;
								}

								temp->type = TOKEN_TEXT;

								temp = temp->next;
							}
						}
					}
				}

				break;

			case TOKEN_QUOTE_SINGLE:

				// Skip if not being handled this pass
				if (!pairings[n->type].mate_type) {
					break;
				}

				// Some of these are actually APOSTROPHEs and should not be paired
				if (!((offset == 0) ||
						char_is_whitespace_or_line_ending_or_punctuation(text[offset - 1]) ||
						char_is_whitespace_or_line_ending_or_punctuation(text[offset + 1]))
				   ) {
					n->type = TOKEN_APOSTROPHE;
					break;
				}

				if (offset && char_is_punctuation(text[offset - 1]) &&
						char_is_alphanumeric(text[offset + 1])) {
					// If possessive apostrophe (e.g. `x`'s)
					if (text[offset + 1] == 's' || text[offset + 1] == 'S') {
						if (char_is_whitespace_or_line_ending_or_punctuation(text[offset + 2])) {
							n->type = TOKEN_APOSTROPHE;
							break;
						}
					}
				}

			// Cascade into standard checks
			case TOKEN_QUOTE_DOUBLE:
			case TOKEN_QUOTE_DOUBLE_ALT:

				// Skip if not being handled this pass
				if (!pairings[n->type].mate_type) {
					break;
				}

				if (!token_closes(p, n, prev, pairings, text, s, stack_start, openers, options)) {
					token_opens(n, pairings, text, s, openers);
				}

				break;

			case TOKEN_UL:
			case TOKEN_STAR:

				// Skip if not being handled this pass
				if (!pairings[n->type].mate_type) {
					break;
				}

				if (((temp = stack_peek(s)) && temp->type == n->type) && (temp->start + temp->len == n->start)) {
					// Preceding character was same as this character, and is an opener.
					// So this is also an opener.
					token_opens(n, pairings, text, s, openers);
				} else if (!(opener = token_closes(p, n, prev, pairings, text, s, stack_start, openers, options))) {
					// If we can't close, then open
					token_opens(n, pairings, text, s, openers);
				} else {
					// This was closing token -- assign strong/emph?
					if (n->next && n->next->type == n->type) {
						// If the opener is preceded by same character as this token is followed by,
						// Then wait until we finish the run
						if (opener->start && text[opener->start - 1] == text[n->start + n->len]) {
							// Wait until next token
						} else {
							// Go ahead
							pair_emphasis_tokens(opener);
						}
					} else {
						// Run is finished
						pair_emphasis_tokens(opener);
					}
				}

				break;


			case TOKEN_PAIR_PAREN:
			case TOKEN_PAIR_BRACKET:
			case TOKEN_PAIR_BRACKET_ABBREVIATION:
			case TOKEN_PAIR_BRACKET_CITATION:
			case TOKEN_PAIR_BRACKET_FOOTNOTE:
			case TOKEN_PAIR_BRACKET_GLOSSARY:
			case TOKEN_PAIR_BRACKET_IMAGE:
			case TOKEN_PAIR_BRACKET_VARIABLE:

			case TOKEN_PAIR_CM_ADD:
			case TOKEN_PAIR_CM_DEL:
			case TOKEN_PAIR_CM_SUB_DEL:
			case TOKEN_PAIR_CM_SUB_ADD:
			case TOKEN_PAIR_CM_COM:
			case TOKEN_PAIR_CM_HI:


			case LINE_TABLE:
			case LINE_TABLE_SEPARATOR:
			case TOKEN_TABLE_CELL:
				if (n->child) {
					analyze_token_chain(p, n->child, pairings, text, c, options);
				}

				break;

			case TOKEN_PAIR_BACKTICK:
			default:
				// Don't descend
				break;

		}

		prev = n;
		n = n->next;
	}

	while (s->size > stack_start) {
		stack_pop(s);
	}
}


void mmd_parse_tokens_block(mmd_node * b, const char * text, read_ctx * c, mmd_node_pool * p, uint32_t options) {
	if (b && b->child && MMD_NODE_IS_BLOCK(b->child)) {
		// This is a parent block, so parse tokens in child nodes
		b = b->child;

		while (b) {
			mmd_parse_tokens_block(b, &text[b->start], c, p, options);

			b = b->next;
		}

		return;
	}

	mmd_node * chain = NULL;

	if (b && b->content) {
		// Block has already been tokenized
		return;
	} else {
		// Tokenize the block

		mmd_tokenizer * z = mmd_tokenizer_new(text, b, c, p, options);

		chain = mmd_tokenizer_accept_token(z, p, options);
		mmd_node * t = chain;

		while (t) {
			if (t->type == TOKEN_TAG) {
				if (
					(text[t->start - 1] != '[') &&
					(text[t->start - 1] != '(')
				) {
					read_ctx_store_tag(c, &text[t->start], t->len);
				}
			}

			t->next = mmd_tokenizer_accept_token(z, p, options);

			// TODO: Debugging only
			if (t && t->next && t->start > t->next->start) {
				mmd_node_describe(b, stderr, text, 0);
				mmd_node_tree_describe(chain, stderr, text, 0);
				fprintf(stderr, "Tokenizing error\n");
			}

			t = t->next;
		}

		mmd_tokenizer_free(z);
	}

	analyze_token_chain(p, chain, pairings1, text, c, options);
	// analyze_token_chain(chain, pairings2, text, c, options);
	analyze_token_chain(p, chain, pairings3, text, c, options);
	analyze_token_chain(p, chain, pairings4, text, c, options);

	if (b) {
		b->content = chain;
	}
}


void mmd_parse_meta_block(const char * text, size_t len, read_ctx * c) {
	c->has_meta = 1;

	const char * start = text;
	const char * cur = start;
	const char * stop = text + len;
	const char * temp;

	meta * m = NULL;

	do {
		m = NULL;
		temp = cur;
		cur += scan_metadata(cur, stop - cur, &m);

		if (m) {
			m->value_start += temp - start;
			read_ctx_store_meta(c, m);
		}
	} while (m && cur < stop);
}


// BLOCK_DEFLIST consists of one or more BLOCK_TERM and one or more BLOCK_DEFINITION
// BLOCK_TERM consists of multiple lines, each of which is actually a TERM
void mmd_parse_tokens_deflist(mmd_node * b, const char * text, read_ctx * c, mmd_node_pool * p, uint32_t options) {
	mmd_node * child = b->child;

	while (child) {
		// Flag leading colon as markup inside definitions
		if (child->type == BLOCK_DEFINITION) {
			c->is_definition = 1;
		}

		mmd_parse_tokens_block(child, &text[child->start], c, p, options);

		c->is_definition = 0;

		child = child->next;
	}
}


/// Parse token chain into table cells
static void analyze_table_row_chain(mmd_node * n, mmd_node_pool * p) {
	mmd_node * first = NULL;
	mmd_node * last = NULL;

	mmd_node * walker = n;

	// Determine starting token of first cell
	if (walker) {
		if (walker->type == TOKEN_TEXT_WHITESPACE) {
			while (walker && walker->type == TOKEN_TEXT_WHITESPACE) {
				walker = walker->next;
			}
		}

		if (walker && walker->type == TOKEN_PIPE) {
			walker->type = TOKEN_TABLE_DIVIDER;
			first = walker->next;
		} else {
			first = walker;
			last = first;
		}

		if (walker) {
			walker = walker->next;
		}
	}

	while (walker) {
		switch (walker->type) {
			case TOKEN_PIPE:
				walker->type = TOKEN_TABLE_DIVIDER;
				mmd_node_prune_graft(p, first, last, TOKEN_TABLE_CELL);
				first = NULL;
				last = NULL;
				break;

			case TOKEN_NL:
			case TOKEN_LINEBREAK:
				break;

			default:
				if (!first) {
					first = walker;
				}

				last = walker;
				break;
		}

		walker = walker->next;
	}

	if (first) {
		mmd_node_prune_graft(p, first, last, TOKEN_TABLE_CELL);
	}
}


/// Find each line within the table block, and parse for cells and cell contents
void mmd_parse_tokens_table(mmd_node * b, const char * text, read_ctx * c, mmd_node_pool * p, uint32_t options) {
	if (b && b->child && MMD_NODE_IS_BLOCK(b->child)) {
		// This is a parent block, so parse tokens in child nodes
		b = b->child;

		while (b) {
			mmd_parse_tokens_table(b, text, c, p, options);

			b = b->next;
		}

		return;
	}

	if (b && b->child && MMD_NODE_IS_LINE(b->child)) {
		//  Reset source text since lines are calculated relative to parent block
		text = &text[b->start];

		// Iterate through each line in the table
		mmd_node * l = b->child;

		// Lines will be attached to row blocks, not the parent blocks
		b->child = NULL;

		mmd_node * rows = NULL;

		while (l) {
			mmd_tokenizer * z = mmd_tokenizer_new(&text[l->start], l, c, p, options);
			mmd_node * chain = mmd_tokenizer_accept_token(z, p, options);
			mmd_node * t = chain;
			mmd_node * next = l->next;
			l->next = NULL;

			while (t) {
				if (t->type == TOKEN_TAG) {
					if (
						(text[t->start - 1] != '[') &&
						(text[t->start - 1] != '(')
					) {
						read_ctx_store_tag(c, &text[t->start], t->len);
					}
				}

				t->next = mmd_tokenizer_accept_token(z, p, options);
				t = t->next;
			}

			mmd_tokenizer_free(z);

			// Divide line into table cells before analyzing
			analyze_table_row_chain(chain, p);

			// Do standard span
			analyze_token_chain(p, chain, pairings1, &text[l->start], c, options);
			// analyze_token_chain(chain, pairings2, &text[l->start], c, options);
			analyze_token_chain(p, chain, pairings3, &text[l->start], c, options);
			analyze_token_chain(p, chain, pairings4, &text[l->start], c, options);

			mmd_node * row = mmd_node_new_parent(p, l, (l->type == LINE_TABLE_SEPARATOR) ? BLOCK_TABLE_SEPARATOR : BLOCK_TABLE_ROW);
			//row->start = l->start;
			row->content = chain;

			if (rows) {
				mmd_node_chain_append(rows, row);
			} else {
				rows = row;
			}

			l = next;

			if (l && l->type == LINE_EMPTY) {
				// NOTE: This empty line node will "disappear".  It won't leak since it is still freed by the line vector
				// mmd_node_chain_append(rows, l);
				next = l->next;
				l->next = NULL;
				l = next;
			}
		}

		b->child = rows;

		return;
	}
}


endnote_def * mmd_parse_tokens_endnote(mmd_node * b, const char * text, size_t len, read_ctx * c, mmd_node_pool * p, uint32_t options) {
	mmd_parse_tokens_block(b, text, c, p, options);

	endnote_def * e = calloc(1, sizeof(endnote_def));

	mmd_node * n = b->content;

	// Skip leading whitespace
	int loop = 1;

	while (loop && n) {
		switch (n->type) {
			case TOKEN_PAIR_BRACKET:
			case TOKEN_PAIR_BRACKET_CITATION:
			case TOKEN_PAIR_BRACKET_FOOTNOTE:
			case TOKEN_PAIR_BRACKET_GLOSSARY:
				loop = 0;
				e->key = md_id_from_text(&text[n->child->next->start], n->next->start - n->child->next->start, true);

				// Skip closer
				n = n->next;
				break;

			default:
				break;
		}

		n = n->next;
	}

	e->text = text;
	e->len = len;
	e->content_node = n;

	return e;
}


/// Used to create HTML compatible id (no spaces)
char * disabled_html_id_from_text(const char * text, size_t len, bool require_odd_count) {
	text_buffer * label = text_buffer_new(len);

	// If text contains [...] use that
	const char * bracket_open = NULL;
	const char * bracket_close = NULL;
	int count = 0;

	const char * cur = text;

	while (cur < text + len) {
		switch (*cur) {
			case '[':
				bracket_open = cur;
				break;

			case ']':
				if (bracket_open && bracket_close == bracket_open - 1) {
					count++;
				} else {
					count = 1;
				}

				bracket_close = cur;
				break;
		}

		cur++;
	}

	if (!(count % 2) && require_odd_count) {
		bracket_open = NULL;
		bracket_close = NULL;
	}

	const char * stop = text + len;

	if (bracket_close && bracket_open && bracket_open < bracket_close) {
		text = bracket_open + 1;
		stop = bracket_close;
	}

	const char * next = text + 1;

	while (text < stop) {

		if ((next < stop) && ((*next & 0xC0) == 0x80)) {
			// Allow multibyte characters
			text_buffer_append_c(label, *text);

			while ((next < stop) && ((*next & 0xC0) == 0x80)) {
				text++;
				text_buffer_append_c(label, *text);
				next++;
			}
		} else {
			switch (*text) {
				case '.':
				case '_':
				case '-':
				case ':':
					// Allowed symbols
					text_buffer_append_c(label, *text);
					break;

				default:
					if (char_is_alphanumeric(*text)) {
						// Allow letters and digits
						text_buffer_append_c(label, tolower(*text));
					}

					break;
			}
		}

		text++;
		next++;
	}

	char * result = label->text;
	text_buffer_free(label, 0);
	return result;
}


/// Used to create HTML compatible id (no spaces)
char * html_id_from_text(const char * text, size_t len, bool require_odd_count) {
	if (0 && require_odd_count) {}

	text_buffer * label = text_buffer_new(len);

	const char * stop = text + len;
	const char * next = text + 1;

	while (text < stop) {

		if ((next < stop) && ((*next & 0xC0) == 0x80)) {
			// Allow multibyte characters
			text_buffer_append_c(label, *text);

			while ((next < stop) && ((*next & 0xC0) == 0x80)) {
				text++;
				text_buffer_append_c(label, *text);
				next++;
			}
		} else {
			switch (*text) {
				case '.':
				case '_':
				case '-':
				case ':':
					// Allowed symbols
					text_buffer_append_c(label, *text);
					break;

				default:
					if (char_is_alphanumeric(*text)) {
						// Allow letters and digits
						text_buffer_append_c(label, tolower(*text));
					}

					break;
			}
		}

		text++;
		next++;
	}

	char * result = label->text;
	text_buffer_free(label, 0);
	return result;
}


/// Create a Markdown id (e.g. for reference links, images, etc.)
/// Spaces are allowed (but collapse multiple spaces into a single space)
/// Trim leading and trailing whitespace
/// Lower case (except for abbreviations)
char * md_id_from_text(const char * text, size_t len, bool require_odd_count) {
	text_buffer * label = text_buffer_new(len);

	// If text contains [...] use that
	const char * bracket_open = NULL;
	const char * bracket_close = NULL;
	int count = 0;

	const char * cur = text;

	while (cur < text + len) {
		switch (*cur) {
			case '[':
				bracket_open = cur;
				break;

			case ']':
				if (bracket_open != NULL) {
					if (bracket_close == bracket_open - 1) {
						count++;
					} else {
						count = 1;
					}

					bracket_close = cur;
				}

				break;
		}

		cur++;
	}

	if (!(count % 2) && require_odd_count) {
		bracket_open = NULL;
		bracket_close = NULL;
	}

	const char * stop = text + len;

	if (bracket_close && bracket_open && bracket_open < bracket_close) {
		cur = bracket_open + 1;
		stop = bracket_close;
	} else {
		cur = text;
	}

	const char * next = cur + 1;

	switch (*cur) {
		case '#':
		case '>':
		case '?':
		case '^':
			text_buffer_append_c(label, *cur);
			cur++;
			next++;
			break;

		default:
			break;
	}

	// Trim leading whitespace
	while (char_is_whitespace(*cur)) {
		cur++;
		next++;
	}

	while (cur < stop) {

		if ((next < stop) && ((*next & 0xC0) == 0x80)) {
			// Allow multibyte characters
			text_buffer_append_c(label, *cur);

			while ((next < stop) && ((*next & 0xC0) == 0x80)) {
				cur++;
				text_buffer_append_c(label, *cur);
				next++;
			}
		} else {
			switch (*cur) {
				case '.':
				case '_':
				case '-':
				case ':':
					// Allowed symbols
					text_buffer_append_c(label, *cur);
					break;

				case ' ':
				case '\t':
				case '\n':
				case '\r':
					// Collapse consecutive whitespace
					text_buffer_append_c(label, ' ');

					while ((next < stop) && char_is_whitespace_or_line_ending(cur[1])) {
						cur++;
						next++;
					}

					break;

				default:
					if (char_is_alphanumeric(*cur) || char_is_punctuation(*cur)) {
						// Allow letters and digits as well as punctuation
						// but lower case
						if (*text == '>') {
							text_buffer_append_c(label, *cur);
						} else {
							text_buffer_append_c(label, tolower(*cur));
						}
					}

					break;
			}
		}

		cur++;
		next++;
	}

	// Trim trailing whitespace
	text_buffer_trim_trailing_whitespace(label);

	char * result = label->text;
	text_buffer_free(label, 0);
	return result;
}


char * definition_name_from_text(const char * text, size_t len) {
	text_buffer * label = text_buffer_new(len);

	// If text contains [...] use that
	const char * bracket_open = NULL;
	const char * bracket_close = NULL;

	const char * cur = text + len - 1;

	while ((cur >= text) && (!bracket_open || !bracket_close)) {
		if (*cur == '[') {
			bracket_open = cur;
		}

		if (*cur == ']') {
			bracket_close = cur;
		}

		cur--;
	}

	const char * stop = text + len;

	if (bracket_close && bracket_open && bracket_open < bracket_close) {
		text = bracket_open + 1;
		stop = bracket_close;
	}

	while (text < stop) {
		text_buffer_append_c(label, tolower(*text));

		text++;
	}

	char * result = label->text;
	text_buffer_free(label, 0);
	return result;
}

