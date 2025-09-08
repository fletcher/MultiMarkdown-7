/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_node_pool.c

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


// Support pthread recursive lock on Linux -- might need to be tweaked for other OS's
#if defined(__APPLE__)
#else
	#define _GNU_SOURCE
#endif


#include <stdlib.h>

#if defined(__WIN32)
	#include <windows.h>
#endif

#include "mmd_node_pool.h"


// Multithreading safety (disabled for performance and should not be necessary)
#define lock	// pthread_mutex_lock(&self->mutex)
#define unlock	// pthread_mutex_unlock(&self->mutex)


// TODO: This number has not been tuned
#define kDefaultSlabCapacity 1024
#define kGrowthMultiplier 2


static void mmd_node_pool_add_slab(mmd_node_pool * self) {
	if (self) {
		lock;

		void * slab = calloc(self->slab_capacity, sizeof(mmd_node));

		if (slab) {
			stack_push(self->slabs, slab);

			// Next mmd_node will come from the beginning of this slab
			self->next = slab;

			// Adjust out of space marker
			self->end = slab + (self->slab_capacity * sizeof(mmd_node));

			// Increase capacity for next allocation
			self->slab_capacity *= kGrowthMultiplier;
		}

		unlock;
	}
}


mmd_node_pool * mmd_node_pool_new(int startingCapacity) {
	mmd_node_pool * p = calloc(1, sizeof(mmd_node_pool));

	if (p) {
		if (startingCapacity <= 0) {
			startingCapacity = kDefaultSlabCapacity;
		}

		p->slab_capacity = startingCapacity;

#if defined(__APPLE__) || defined(__WIN32)
		p->mutex = (pthread_mutex_t) PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
		p->mutex = (pthread_mutex_t) PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

		p->slabs = stack_new(64);

		mmd_node_pool_add_slab(p);
	}

	return p;
}


/// Free the existing slabs, but leave slab_capacity at current value.
/// If an object pool is reused it will then allocate larger slabs to begin with.
void mmd_node_pool_drain(mmd_node_pool * self) {
	if (self) {
		lock;

		while (self->slabs->size) {
			void * slab = stack_pop(self->slabs);
			free(slab);
		}

		self->next = NULL;
		self->end = NULL;

		unlock;
	}
}


void mmd_node_pool_free(mmd_node_pool * self) {
	if (self) {
		lock;

		mmd_node_pool_drain(self);

		stack_free(self->slabs);

#if defined(__WIN32)
		CloseHandle(&self->mutex);
#else
		pthread_mutex_destroy(&self->mutex);
#endif

		unlock;

		free(self);
	}
}


mmd_node * mmd_node_pool_allocate(mmd_node_pool * self) {
	mmd_node * n = NULL;

	if (self) {
		lock;

		if (self->next == self->end) {
			mmd_node_pool_add_slab(self);
		}

		if (self->next < self->end) {
			n = self->next;
			self->next += sizeof(mmd_node);
		}

		unlock;
	}

	return n;
}

