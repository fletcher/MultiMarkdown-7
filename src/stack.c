/**

	ftp-stdlib -- Common files across multiple projects
		https://github.com/fletcher/ftp-stdlib

	@file stack.c

	@brief Create a dynamic array that stores pointers in a LIFO order.


	@author	Fletcher T. Penney
	@bug

 **/

/*

	Copyright © 2016-2020 Fletcher T. Penney.


	MIT License

	Copyright (c) 2016-2020 Fletcher T. Penney

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


#include <stdlib.h>
#include <string.h>

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
	#include <windows.h>
#endif

#include "stack.h"

#ifdef TEST
	#include "CuTest.h"
#endif

#define kStackStartingSize 64

// Multithreading safety (disabled for performance and should not be necessary)
#define lock	// pthread_mutex_lock(&self->mutex)
#define unlock	// pthread_mutex_unlock(&self->mutex)


/// Create a new stack with dynamic storage with an
/// initial capacity (0 to use default capacity)
stack * stack_new(int startingSize) {
	stack * s = malloc(sizeof(stack));

	if (s) {
		if (startingSize <= 0) {
			startingSize = kStackStartingSize;
		}

#if (defined(__WIN32) || defined(__WIN32__) || defined(_MSC_VER))
#elif defined(__APPLE__)
		s->mutex = (pthread_mutex_t) PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
		s->mutex = (pthread_mutex_t) PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

		s->element = malloc(sizeof(stack_data_type) * startingSize);

		if (!s->element) {
			free(s);
			return NULL;
		}

		s->size = 0;
		s->capacity = startingSize;
	}

	return s;
}


/// Free the stack
void stack_free(stack * self) {
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


#ifdef TEST
void Test_stack_new(CuTest * tc) {
	stack * s = stack_new(0);
	CuAssertIntEquals(tc, kStackStartingSize, (int)s->capacity);
	CuAssertIntEquals(tc, 0, (int)s->size);
	stack_free(s);

	s = stack_new(-1);
	CuAssertIntEquals(tc, kStackStartingSize, (int)s->capacity);
	CuAssertIntEquals(tc, 0, (int)s->size);
	stack_free(s);

	s = stack_new(10);
	CuAssertIntEquals(tc, 10, (int)s->capacity);
	CuAssertIntEquals(tc, 0, (int)s->size);
	stack_free(s);
}

#endif


/// Add a new pointer to the stack
void stack_push(stack * self, stack_data_type element) {
	if (self) {
		lock;

		if (self->size == self->capacity) {
			self->capacity *= 2;
			self->element = realloc(self->element, self->capacity * sizeof(stack_data_type));
		}

		self->element[self->size++] = element;

		unlock;
	}
}


/// Pop the top item off the stack
stack_data_type stack_pop(stack * self) {
	stack_data_type last = NULL;

	if (self) {
		lock;

		last = stack_peek(self);

		if (self->size != 0) {
			self->size--;
		}

		unlock;
	}

	return last;
}


/// Peek at the top item on the stack
stack_data_type stack_peek(stack * self) {
	stack_data_type result = NULL;

	if (self) {
		lock;

		if (self->size > 0) {
			result = self->element[(self->size) - 1];
		}

		unlock;
	}

	return result;
}


/// Peek at a specific index in the stack
stack_data_type stack_peek_index(stack * self, size_t index) {
	stack_data_type result = NULL;

	if (self) {
		lock;

		if (index < self->size) {
			result = self->element[index];
		}

		unlock;
	}

	return result;
}


#ifdef TEST
void Test_stack_push_pop(CuTest * tc) {
	stack * s = stack_new(1);

	char * s1 = "foo";
	char * s2 = "bar";
	char * s3 = "bat";

	stack_push(s, s1);
	stack_push(s, s2);
	stack_push(s, s3);

	CuAssertIntEquals(tc, 3, (int)s->size);
	CuAssertIntEquals(tc, 4, (int)s->capacity);

	CuAssertStrEquals(tc, "bat", stack_peek(s));
	CuAssertStrEquals(tc, "foo", stack_peek_index(s, 0));

	CuAssertStrEquals(tc, "bat", stack_pop(s));
	CuAssertStrEquals(tc, "bar", stack_pop(s));
	CuAssertStrEquals(tc, "foo", stack_pop(s));

	CuAssertIntEquals(tc, 0, (int)s->size);

	stack_free(s);
}

#endif


/// Sort array using specified compare_function
void stack_sort(stack * self, int (* compare_function)(const void *, const void *)) {
	if (self) {
		lock;

		qsort(self->element, self->size, sizeof(stack_data_type), compare_function);

		unlock;
	}
}

