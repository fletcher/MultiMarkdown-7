/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file _generate_test_emph.c

	@brief Generate test suite for strong and emph parsing


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

#define F(i,n) for(int i= 0;i<n;i++)

char * s[] = {
	"****",
	"____"
};



int main(int argc, char * const argv[]) {
	fprintf(stdout, "title:  Emphasis Test\nlatexconfig:    article\n\n");

	// Basic
	int c = 1;

	fprintf(stdout, "# %d #\n\n", c++);

	F(i, 2) {
		F(j, 9) {
			fprintf(stdout, "%.*sfoo%.*s\n\n", 1 + j / 3, s[i], 1 + j % 3, s[i]);
		}
	}

	fprintf(stdout, "# %d #\n\n", c++);

	F(i, 2) {
		F(j, 9) {
			fprintf(stdout, "%.*sfoo bar%.*s\n\n", 1 + j / 3, s[i], 1 + j % 3, s[i]);
		}
	}

	fprintf(stdout, "# %d #\n\n", c++);

	F(i, 2) {
		F(j, 9) {
			fprintf(stdout, "%.*sfoo%.*s bar\n\n", 1 + j / 3, s[i], 1 + j % 3, s[i]);
		}
	}

	fprintf(stdout, "# %d #\n\n", c++);

	F(i, 2) {
		F(j, 9) {
			fprintf(stdout, "foo %.*sbar%.*s\n\n", 1 + j / 3, s[i], 1 + j % 3, s[i]);
		}
	}

	fprintf(stdout, "# %d #\n\n", c++);

	F(i, 2) {
		F(j, 9) {
			fprintf(stdout, "%.*sfoo %.*sbar%.*s foo%.*s\n\n", 1 + j / 3, s[i], 1 + j % 3, s[i], 1 + j % 3, s[i], 1 + j / 3, s[i]);
		}
	}


	// Intraword
	fprintf(stdout, "# %d #\n\n", c++);

	F(i, 2) {
		F(j, 9) {
			fprintf(stdout, "%.*sfoo%.*sbar\n\n", 1 + j / 3, s[i], 1 + j % 3, s[i]);
		}
	}

	fprintf(stdout, "# %d #\n\n", c++);

	F(i, 2) {
		F(j, 9) {
			fprintf(stdout, "foo%.*sbar%.*s\n\n", 1 + j / 3, s[i], 1 + j % 3, s[i]);
		}
	}
}
