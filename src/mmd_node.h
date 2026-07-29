/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_node.h

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


#ifndef MMD_NODE_libMultiMarkdown7_H
#define MMD_NODE_libMultiMarkdown7_H

#include "libMultiMarkdown7.h"
#include "mmd_node_pool.h"

#define kRelativeStarts 1

mmd_node * mmd_node_new(mmd_node_pool * p, unsigned char type, size_t start, size_t len);
mmd_node * mmd_node_copy(mmd_node_pool * p, mmd_node * original);
mmd_line_node * mmd_line_node_new(unsigned char type, size_t start, size_t len, size_t c_start, size_t c_len);

void mmd_node_chain_append(mmd_node * start, mmd_node * n);
void mmd_node_prepend_child(mmd_node * parent, mmd_node * child);
void mmd_node_append_child(mmd_node * parent, mmd_node * child);
mmd_node * mmd_node_new_parent(mmd_node_pool * p, mmd_node * child, unsigned char type);
/// Given a start/stop point in a chain, create a new container token that contains
/// this section of the chain
void mmd_node_prune_graft(mmd_node_pool * p, mmd_node * first, mmd_node * last, unsigned char container_type);
void mmd_node_graft(mmd_node * donor, mmd_node * recipient);
void mmd_node_split(mmd_node_pool * p, mmd_node * n, size_t offset);

void mmd_node_describe(mmd_node * n, FILE * stream, const char * text, size_t offset);
void mmd_node_tree_describe(mmd_node * n, FILE * stream, const char * text, size_t offset);

void mmd_node_tree_describe_hash(mmd_node * n, FILE * stream);
uint32_t mmd_hash_node_tree(mmd_node * n);

#endif
