/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_node.c

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


#include <stdio.h>
#include <stdlib.h>

#include "mmd_node.h"


#ifdef TEST
	#include "CuTest.h"
#endif


#define F(i,n) for(int i= 0;i<n;i++)

mmd_node * mmd_node_new(mmd_node_pool * p, unsigned char type, size_t start, size_t len) {
	mmd_node * n = NULL;

	if (p) {
		n = mmd_node_pool_allocate(p);
	} else {
		n = calloc(1, sizeof(mmd_node));
	}

	if (n) {
		n->type = type;
		n->start = start;
		n->len = len;

		n->tail = n;
	}

	return n;
}


mmd_node * mmd_node_copy(mmd_node_pool * p, mmd_node * original) {
	mmd_node * n = mmd_node_new(p, original->type, original->start, original->len);

	return n;
}


mmd_line_node * mmd_line_node_new(unsigned char type, size_t start, size_t len, size_t c_start, size_t c_len) {
	mmd_line_node * n = calloc(1, sizeof(mmd_line_node));

	if (n) {
		n->general.type = type;
		n->general.start = start;
		n->general.len = len;
		n->general.tail = (mmd_node *)n;

		n->c_start = c_start;
		n->c_len = c_len;

		if (
			(c_start > len) ||
			(c_len > len)
		) {
			fprintf(stderr, "Line Node Error\n");
		}
	}

	return n;
}


/// XOR version of djb2 algorithm
///		https://stackoverflow.com/questions/9616296/whats-the-best-hash-for-utf-8-strings
/// Hash each byte of multibyte integer separately
///		https://stackoverflow.com/questions/7787423/c-get-nth-byte-of-integer

/// Recursively calculate hash for node and its children
uint32_t mmd_hash_node(mmd_node * n) {
	uint32_t hash = 5381;

	// Hash type of n
	hash = ((hash << 5) + hash) ^ n->type;

	// Start with child nodes
	mmd_node * w = n->child;

	while (w) {
		uint32_t child = mmd_hash_node(w);

		// Hash lowest two bytes of w->start
		// (which will be relative to the start of n)
		hash = ((hash << 5) + hash) ^ ((w->start >> (8 * 0)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((w->start >> (8 * 1)) & 0xff);

		// Hash the four bytes of w->hash
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 0)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 1)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 2)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 3)) & 0xff);

		w = w->next;
	}

	// Then hash content nodes
	w = n->content;

	while (w) {
		uint32_t child = mmd_hash_node(w);

		// Hash lowest two bytes of w->start
		// (which will be relative to the start of n)
		hash = ((hash << 5) + hash) ^ ((w->start >> (8 * 0)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((w->start >> (8 * 1)) & 0xff);

		// Hash the four bytes of w->hash
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 0)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 1)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 2)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 3)) & 0xff);

		w = w->next;
	}


	// Hash lowest two bytes of n->len
	hash = ((hash << 5) + hash) ^ ((n->len >> (8 * 0)) & 0xff);
	hash = ((hash << 5) + hash) ^ ((n->len >> (8 * 1)) & 0xff);

	n->hash = hash;

	return hash;
}


/// Recursively calculate hash for node tree
/// (It also returns an overall hash for the tree)
uint32_t mmd_hash_node_tree(mmd_node * n) {
	uint32_t hash = 5381;

	while (n) {
		uint32_t child = mmd_hash_node(n);

		// Hash lowest two bytes of w->start
		// (which will be relative to the start of n)
		hash = ((hash << 5) + hash) ^ ((n->start >> (8 * 0)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((n->start >> (8 * 1)) & 0xff);

		// Hash the four bytes of w->hash
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 0)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 1)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 2)) & 0xff);
		hash = ((hash << 5) + hash) ^ ((child >> (8 * 3)) & 0xff);

		n = n->next;
	}

	return hash;
}


void mmd_node_free(mmd_node * n) {
	if (n) {
		// Don't free vector line nodes
		if (MMD_NODE_IS_BLOCK(n)) {
			switch (n->type) {
				case BLOCK_BLOCKQUOTE:
				case BLOCK_DEFINITION:
				case BLOCK_DEFLIST:
				case BLOCK_DEF_CITATION:
				case BLOCK_DEF_FOOTNOTE:
				case BLOCK_DEF_GLOSSARY:
				case BLOCK_LIST_BULLETED:
				case BLOCK_LIST_BULLETED_LOOSE:
				case BLOCK_LIST_ENUMERATED:
				case BLOCK_LIST_ENUMERATED_LOOSE:
				case BLOCK_LIST_ITEM:
				case BLOCK_LIST_ITEM_TIGHT:
				case BLOCK_TABLE:
				case BLOCK_TABLE_HEADER:
				case BLOCK_TABLE_SECTION:
					mmd_node_tree_free(n->child);
					break;

				default:
					break;
			}
		} else {
			mmd_node_tree_free(n->child);
		}

		mmd_node_tree_free(n->content);

		free(n);

		// if (MMD_NODE_IS_LINE(n)) {
		// 	n->child = NULL;
		// 	n->content = NULL;
		// } else {
		// 	free(n);
		// }
	}
}


void mmd_node_tree_free(mmd_node * n) {
	mmd_node * w;

	while (n) {
		w = n->next;

		mmd_node_free(n);

		n = w;
	}
}


void mmd_node_chain_append(mmd_node * start, mmd_node * n) {
	if ((start == NULL) || (n == NULL)) {
		return;
	}

	start->tail->next = n;
	start->tail = n->tail;
}


void mmd_node_append_child(mmd_node * parent, mmd_node * child) {
	if (parent && child) {
		if (parent->child) {
			mmd_node_chain_append(parent->child, child);
		} else {
			parent->child = child;
		}

		while (child) {
#ifdef kRelativeStarts
			child->start -= parent->start;
#endif
			child = child->next;
		}

#ifdef kRelativeStarts
		parent->len = parent->child->tail->start + parent->child->tail->len;
#else
		parent->len = parent->child->tail->start + parent->child->tail->len - parent->start;
#endif
	}
}


void mmd_node_graft(mmd_node * donor, mmd_node * recipient) {
	if (donor && recipient) {
		if (recipient->child) {
			mmd_node_chain_append(recipient->child, donor->child);
		} else {
			recipient->child = donor->child;
		}

		recipient->child->tail = donor->child->tail;

#ifdef kRelativeStarts
		mmd_node * w = donor->child;

		while (w) {
			w->start = w->start + donor->start - recipient->start;
			w = w->next;
		}

#endif

		mmd_node * tail = recipient->child->tail;

#ifdef kRelativeStarts
		recipient->len = tail->start + tail->len;
#else
		recipient->len = tail->start + tail->len - recipient->len;
#endif
	}
}


void mmd_node_split(mmd_node_pool * p, mmd_node * n, size_t offset) {
	if (n && n->len > offset) {
		mmd_node * new = mmd_node_new(p, n->type, n->start + offset, n->len - offset);
		n->len = offset;
		new->next = n->next;
		n->next = new;

		if (n->tail == n) {
			n->tail = new;
		}
	}
}


void mmd_node_prepend_child(mmd_node * parent, mmd_node * child) {
	if (parent && child) {
		if (parent->child) {
			child->next = parent->child;
			child->tail = parent->child->tail;
			parent->child = child;
		} else {
			parent->child = child;
		}

		parent->start = child->start;

#ifdef kRelativeStarts
		child->start = 0;

		mmd_node * w = child->next;

		while (w) {
			w->start += child->len;
			w = w->next;
		}

		parent->len = child->tail->start + child->tail->len;
#else
		parent->len = child->tail->start + child->tail->len - parent->start;
#endif
	}
}


mmd_node * mmd_node_new_parent(mmd_node_pool * p, mmd_node * child, unsigned char type) {
	if (child == NULL) {
		return mmd_node_new(p, type, 0, 0);
	}

	mmd_node * n = mmd_node_new(p, type, child->start, 0);

	if (n) {
		n->child = child;
#ifdef kRelativeStarts
		child->start = 0;
#endif

		while (child->next != NULL) {
			child = child->next;
#ifdef kRelativeStarts
			child->start -= n->start;
#endif
		}

#ifdef kRelativeStarts
		n->len = child->start + child->len;
#else
		n->len = child->start + child->len - n->start;
#endif
	}

	return n;
}


/// Given a start/stop point in a chain, create a new container token that contains
/// this section of the chain
void mmd_node_prune_graft(mmd_node_pool * p, mmd_node * first, mmd_node * last, unsigned char container_type) {
	if (first == NULL || last == NULL) {
		return;
	}

	mmd_node * next = last->next;

	// Duplicate first token (we will convert current first token into the new container)
	mmd_node * new_child = mmd_node_copy(p, first);
	new_child->next = first->next;
	new_child->tail = last;

	// Swap last node to new child if necessary
	if (first == last) {
		last = new_child;
	}

	// Prior first token will be new container
	first->child = new_child;
	first->type = container_type;
	first->next = next;

	if (first->next == NULL) {
		first->tail = first;
	}

	// Disconnect last
	last->next = NULL;
}


void mmd_node_tree_print(mmd_node * n, FILE * stream, char marker, unsigned short depth, const char * text, size_t offset);


void mmd_node_print(mmd_node * n, FILE * stream, char marker, unsigned short depth, const char * text, size_t offset) {
	if (n != NULL) {
		for (int i = 0; i < depth; ++i) {
			fprintf(stream, "\t");
		}

		if (text == NULL) {
			fprintf(stream, "%c (%d) %zu:%zu\n", marker, n->type, n->start, n->len);
		} else {
			fprintf(stream, "%c (%d) %zu:%zu\t'%.*s'\n", marker, n->type, n->start, n->len, (int)n->len, &text[n->start + offset]);
			//fprintf(stream, "%.*s", (int)n->len, &text[n->start]);
		}

		if (n->child != NULL) {
			if (MMD_NODE_IS_BLOCK(n)) {
				mmd_node_tree_print(n->child, stream, '*', depth + 1, text, offset + n->start);
			} else {
				mmd_node_tree_print(n->child, stream, '+', depth + 1, text, offset);
			}
		}

		if (n->content != NULL) {
			mmd_node_tree_print(n->content, stream, '-', depth + 1, text, offset + n->start);
		}
	}
}


void mmd_node_tree_print(mmd_node * n, FILE * stream, char marker, unsigned short depth, const char * text, size_t offset) {
	while (n != NULL) {
		mmd_node_print(n, stream, marker, depth, text, offset);

		n = n->next;
	}
}


void mmd_node_describe(mmd_node * n, FILE * stream, const char * text, size_t offset) {
	mmd_node_print(n, stream, '*', 0, text, offset);

//	if (n->content) {
//		mmd_node_tree_print(n->content, stream, 0, text, offset);
//	}
}


void mmd_node_tree_describe(mmd_node * n, FILE * stream, const char * text, size_t offset) {
	fprintf(stream, "=====>\n");
	mmd_node_tree_print(n, stream, '*', 0, text, offset);
	fprintf(stream, "<=====\n");
}


void mmd_node_tree_print_hash(mmd_node * n, FILE * stream, unsigned short depth);


void mmd_node_print_hash(mmd_node * n, FILE * stream, unsigned short depth) {
	if (n != NULL) {
		for (int i = 0; i < depth; ++i) {
			fprintf(stream, "\t");
		}

		fprintf(stream, "* (%d) %zu:%zu - %u\n", n->type, n->start, n->len, n->hash);
		// fprintf(stream, "* (%d) %u\n", n->type, n->hash);

		if (n->child != NULL) {
			mmd_node_tree_print_hash(n->child, stream, depth + 1);
		}

		if (n->content != NULL) {
			mmd_node_tree_print_hash(n->content, stream, depth + 1);
		}
	}
}


void mmd_node_tree_print_hash(mmd_node * n, FILE * stream, unsigned short depth) {
	while (n != NULL) {
		mmd_node_print_hash(n, stream, depth);

		n = n->next;
	}
}


void mmd_node_describe_hash(mmd_node * n, FILE * stream) {
	mmd_node_print_hash(n, stream, 0);
}


void mmd_node_tree_describe_hash(mmd_node * n, FILE * stream) {
	if (n) {
		mmd_node_tree_print_hash(n, stream, 0);
	}
}

