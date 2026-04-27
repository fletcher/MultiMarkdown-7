/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file _generate_test_reference.c

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


#define F(i,n) for(int i= 0;i<n;i++)

char * s[] = {
	"****",
	"____"
};


char * marker[] = {
	"![",		// Matches CommonMark
	"[",		// Matches CommonMark

	"[^",		// Matches MultiMarkdown v6
	"[#",		// Matches MultiMarkdown v6

	"[>",		// Changed from MultiMarkdown v6
	"[?",		// Changed from MultiMarkdown v6
};


char * label[] = {
	"foo",
	"bar",
	"bat",
	"baz"
};

int main(int argc, char * const argv[]) {
	fprintf(stdout, "title:  Reference Test\nlatexconfig:    article\n\n");

	F(j, sizeof(label) / sizeof(label[0])) {
		F(i, sizeof(marker) / sizeof(marker[0])) {
			fprintf(stdout, "%s%s]\n", marker[i], label[j]);
		}
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "\n");

	int c = 0;

	F(j, 2) {
		// 1 reference at a time vs chunk them together
		F(k, 2) {
			// Title vs no title
			F(i, sizeof(marker) / sizeof(marker[0])) {
				// Skip image
				if (i) {
					if (k) {
						fprintf(stdout,  "%s%s]: *%s* \"Title %s %d\"%s", marker[i], label[c], label[c], label[c], i, (j) ? "\n\n" : "\n");
					} else {
						fprintf(stdout,  "%s%s]: *%s*%s", marker[i], label[c], label[c], (j) ? "\n\n" : "\n");
					}
				}
			}

			c++;
		}
	}

	// Whitespace
	F(j, sizeof(label) / sizeof(label[0])) {
		F(i, sizeof(marker) / sizeof(marker[0])) {
			if (i) {
				fprintf(stdout, "%s %s1]\n", marker[i], label[j]);
				fprintf(stdout, "%s%s1 ]\n", marker[i], label[j]);
				fprintf(stdout, "%s %s1 ]\n\n", marker[i], label[j]);
			}
		}
	}

	F(j, sizeof(label) / sizeof(label[0])) {
		F(i, sizeof(marker) / sizeof(marker[0])) {
			// Skip image
			if (i) {
				switch (j) {
					case 0:
						fprintf(stdout,  "%s %s1]: **%s1**\n", marker[i], label[j], label[j]);
						break;

					case 1:
						fprintf(stdout,  "%s%s1 ]: **%s1**\n", marker[i], label[j], label[j]);
						break;

					case 2:
						fprintf(stdout,  "%s %s1 ]: **%s1**\n", marker[i], label[j], label[j]);
						break;
				}
			}
		}
	}
}
