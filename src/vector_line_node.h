/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file vector_line_node.h

	@brief


	@author	Fletcher T. Penney
	@bug

**/

/*

	MIT License

	Copyright (c) 2025 Fletcher T. Penney

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


#ifndef VECTOR_LINE_NODE_LIBMULTIMARKDOWN7_H
#define VECTOR_LINE_NODE_LIBMULTIMARKDOWN7_H


#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	#include <pthread.h>
#endif


typedef struct {
	size_t				size;
	size_t				capacity;

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	pthread_mutex_t		mutex;
#endif

	mmd_line_node *		element;
} vector_line_node;


vector_line_node * vector_line_node_new(int startingCapacity);
void vector_line_node_free(vector_line_node * v);
void vector_line_node_add(vector_line_node * v, mmd_line_node l);
mmd_line_node vector_line_node_peek(vector_line_node * v);
mmd_line_node vector_line_node_peek_index(vector_line_node * v, size_t index);
void vector_line_node_describe(vector_line_node * self, FILE * stream);

#endif
