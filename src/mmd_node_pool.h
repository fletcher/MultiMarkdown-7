/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_node_pool.h

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


#ifndef MMD_NODE_POOL_LIBMULTIMARKDOWN7_H
#define MMD_NODE_POOL_LIBMULTIMARKDOWN7_H


#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	#include <pthread.h>
#endif


#include "libMultiMarkdown7.h"
#include "stack.h"

struct mmd_node_pool {
	int					slab_capacity;
	stack *				slabs;
	void *				next;
	void *				end;

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	pthread_mutex_t		mutex;
#endif
};


typedef struct mmd_node_pool mmd_node_pool;


mmd_node_pool * mmd_node_pool_new(int startingCapacity);

/// Free the existing slabs, but leave slab_capacity at current value.
/// If an object pool is reused it will then allocate larger slabs to begin with.
void mmd_node_pool_drain(mmd_node_pool * self);

void mmd_node_pool_free(mmd_node_pool * p);

mmd_node * mmd_node_pool_allocate(mmd_node_pool * p);

#endif
