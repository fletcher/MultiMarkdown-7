/**

	libMultiMarkdown7 -- C parser for Markdown with additional features and multiple output formats.

	@file _generate_test_structure.c

	@brief Generate test suite for overall structure


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


char * line[] = {
	"# Header 1 #",
	"Plain text.",
	"* Bulleted item",
	"1. Enumerated item",
	"=========",
	"---------",
	"	Indented line",
	"> Blockquote line",
	"---",

	"	# Indented Header 1 #",
	"	* Indented Bulleted item",
	"	1. Indented Enumerated item",
	"	=========",
	"	---------",
	"	> Indented Blockquote line",

	""
};


/// Increment array by FTP
int increment_array(int * arr, int start, int end, int s) {
	if (start == end) {
		return 0;
	}

	arr[end - start - 1]++;

	if (arr[end - start - 1] == s) {
		if (!increment_array(arr, start + 1, end, s)) {
			return 0;
		}

		arr[end - start - 1] = 0;
	}

	return 1;
}


/// Cycle through n "digit" number consisting of "s" symbols (like an odometer)
void cycle_array(int n, int s, void * y, void * z, void(*cb)(void * y, void * z, int * arr, int size)) {
	int * temp = calloc(n, sizeof(int));

	cb(y, z, temp, n);

	while (increment_array(temp, 0, n, s)) {
		cb(y, z, temp, n);
	}

	free(temp);
}


/// Example callback function to print the array permutation/combination (y and z ignored)
void print_array_cb(void * y, void * z, int * arr, int size) {
	// Certain combinations, at least for now, are not considered invalid
	switch (size) {
		case 3: {
			switch (arr[0]) {
				case 2:
				case 3: {
					switch (arr[1]) {
						case 9:
						case 12:
						case 13: {
							switch (arr[2]) {
								case 1:
								case 4:
								case 6:
								case 12:
									return;
							}
						}
					}
				}
			}
		}
		break;

		case 4: {
			switch (arr[1]) {
				case 2:
				case 3: {
					switch (arr[2]) {
						case 9:
						case 12:
						case 13: {
							switch (arr[3]) {
								case 1:
								case 4:
								case 6:
								case 12:
									return;
							}
						}
					}
				}
			}

			switch (arr[0]) {
				case 2:
				case 3: {
					switch (arr[1]) {
						case 9:
						case 12:
						case 13: {
							switch (arr[2]) {
								case 1:
								case 4:
								case 6:
								case 12:
									return;
							}
						}
					}
				}
			}
		}

	}

	F(i, size) {
		printf("%d%c", arr[i], (i == size - 1) ? '\n' : '-');
	}

	printf("\n");

	F(i, size) {
		printf("%s\n", line[arr[i]]);
	}

	printf("\n");
	printf("\n");
}


int main(int argc, char * const argv[]) {
	int n = 3;
	int s = sizeof(line) / sizeof(line[0]);

	switch (argc) {
		case 2:
			n = atoi(argv[1]);
			break;
	}

	cycle_array(n, s, NULL, NULL, &print_array_cb);
}


