/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file vector_line_node.c

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


// Support pthread recursive lock on Linux -- might need to be tweaked for other OS's
#if defined(__APPLE__)
#elif (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#else
	#define _GNU_SOURCE
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	#include <windows.h>
#endif

#include "libMultiMarkdown.h"

#include "vector_line_node.h"


#define kVectorStartingCapacity 4
#define kGrowthMultiplier 2

// Multithreading safety (disabled for performance and should not be necessary)
#define lock	// pthread_mutex_lock(&self->mutex)
#define unlock	// pthread_mutex_unlock(&self->mutex)

#define F(i,n) for(int i= 0;i<n;i++)


vector_line_node * vector_line_node_new(int startingCapacity) {
	vector_line_node * v = calloc(1, sizeof(vector_line_node));

	if (v) {
		if (startingCapacity <= 0) {
			startingCapacity = kVectorStartingCapacity;
		}

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#elif defined(__APPLE__)
		v->mutex = (pthread_mutex_t) PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
		v->mutex = (pthread_mutex_t) PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

		v->element = calloc(startingCapacity, sizeof(mmd_line_node));

		if (!v->element) {
			free(v);
			return NULL;
		}

		v->size = 0;
		v->capacity = startingCapacity;
	}

	return v;
}


void vector_line_node_free(vector_line_node * self) {
	if (self) {
		lock;

		free(self->element);

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
		// CloseHandle(&self->mutex);
#else
		pthread_mutex_destroy(&self->mutex);
#endif

		unlock;

		free(self);
	}
}


void vector_line_node_add(vector_line_node * self, mmd_line_node l) {
	if (self) {
		lock;

		if (self->size == self->capacity) {
			void * new = realloc(self->element, self->capacity * kGrowthMultiplier * sizeof(mmd_line_node));

			if (new) {
				self->element = new;
				self->capacity *= kGrowthMultiplier;
			} else {
				fprintf(stderr, "Reallocation error\n");
				unlock;
				return;
			}
		}

		self->element[self->size++] = l;

		unlock;
	}
}


mmd_line_node vector_line_node_peek(vector_line_node * self) {
	mmd_line_node l = {0};

	if (self) {
		lock;

		if (self->size) {
			l = self->element[self->size - 1];
		}

		unlock;
	}

	return l;
}


mmd_line_node vector_line_node_peek_index(vector_line_node * self, size_t index) {
	mmd_line_node l = {0};

	if (self) {
		lock;

		if (index < self->size) {
			l = self->element[index];
		}

		unlock;
	}

	return l;
}


void vector_line_node_describe(vector_line_node * self, FILE * stream) {
	if (self) {
		lock;

		F(i, (int)self->size) {
			fprintf(stream, "line #%d (%d) => %zu:%zu; next => %zu", i, self->element[i].general.type, self->element[i].general.start, self->element[i].general.len,
					((char *) self->element[i].general.next - (char *) self->element) / sizeof(mmd_line_node));
		}
	}
}


// TODO: Add binary search for a character offset?
