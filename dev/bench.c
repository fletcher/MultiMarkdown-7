/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file main.c

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
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


#ifdef TEST
	#include "CuTest.h"
#endif


#define F(i,n) for(int i= 0;i<n;i++)


char * commands[][2] = {
	"multimarkdown6 ", "mmd6",								// Basic MMD 6 parsing to HTML
	"../build/multimarkdown parse ", "mmd7",				// Basic MMD 7 parsing to HTML
	"../build/multimarkdown parse -c ", "mmd7-c",			// MMD 7 HTML compatibility mode
	"../build/multimarkdown parse -t latex ", "mmd7-l",		// MMD 7 LaTeX parsing
	"cmark ", "cmark",										// CommonMark
	"md2html ", "md4c",										// MD4C
};


char * files[] = {
	"bench/speed1024.md",
	"bench/long-block-multiline.md",
	"bench/long-block-oneline.md",
	"bench/many-atx-headers.md",
	"bench/many-blanks.md",
	"bench/many-emphasis.md",
	"bench/many-fenced-code-blocks.md",
	"bench/many-links.md",
	"bench/many-paragraphs.md",
	"bench/deep-blockquote.md",
	"bench/shallow-blockquote.md",
	"bench/small-file.md"			// This should always be last in the list
};


char * args[] = {
	"parse",
	"parse -c",
	"parse -b",
	"parse -t latex",
	"ast",
	"hash",
};


// https://stackoverflow.com/questions/64893834/measuring-elapsed-time-using-clock-gettimeclock-monotonic
static int64_t difftimespec_us(const struct timespec after, const struct timespec before) {
	return ((int64_t)after.tv_sec - (int64_t)before.tv_sec) * (int64_t)1000000
		   + ((int64_t)after.tv_nsec - (int64_t)before.tv_nsec) / 1000;
}


int main(int argc, char * const argv[]) {
	int option;
	int err = 0;
	char action = '\0';

	// Set offset to 1 if we want an "action" immediately following the program when called
	// e.g.  ./foo bar -x -y -z
	int offset = 0;

	if (argc > 1) {
		if (argv[1][0] != '-' || argv[1][1] == '-') {
			// Skip action or long argument (e.g. `--help`)
			offset = 1;
		}
	}


	// Read short options
	while ((option = getopt(argc - offset, &argv[offset], ":h")) != -1) {
		switch (option) {
			case 'h':
				// help -- display usage
				err = -1;
				break;
		}
	}


	// Is an action required?
	if (!err && !offset) {
		fprintf(stderr, "%s: action missing\n", argv[0]);
		err = 1;
	}

	if (err == 0 && offset) {
		if (argc < offset + 1) {
			// Required action missing
			err = 1;
		} else if (strcmp(argv[1], "--help") == 0) {
			// help -- display usage
			err = -1;
		} else if (strcmp(argv[1], "build") == 0) {
			action = 'b';
		} else if (strcmp(argv[1], "run") == 0) {
			action = 'r';
		} else {
			fprintf(stderr, "%s: action not recognized -- %s\n", argv[0], argv[1]);
			err = 1;
		}
	}


	// Read arguments
	for (optind += offset; optind < argc; optind++) {
		fprintf(stderr, "%s: argument not recognized -- %s\n", argv[0], argv[optind]);
	}


	if (err) {
		// Error
		fprintf(stderr, "usage: %s {build|run} [-h]\n", argv[0]);
	} else {
		// Proceed
		switch (action) {
			case 'b': {
				system("cat ../tests/MMD7Tests/Markdown\\ Syntax.text > bench/speed1024.md");

				F(i, 1023) {
					system("cat ../tests/MMD7Tests/Markdown\\ Syntax.text >> bench/speed1024.md");
				}

				FILE * out;

				out = fopen("bench/long-block-multiline.md", "w");
				F(i, 1000000) {
					fprintf(out, "foo\n");
				}

				fclose(out);

				out = fopen("bench/long-block-oneline.md", "w");
				F(i, 1000000) {
					F(j, 10) {
						fprintf(out, "foo ");
					}
				}

				fclose(out);

				out = fopen("bench/many-atx-headers.md", "w");
				F(i, 1000000) {
					fprintf(out, "###### foo\n");
				}

				fclose(out);


				out = fopen("bench/many-blanks.md", "w");
				F(i, 1000000) {
					F(j, 10) {
						fprintf(out, "\n");
					}
				}

				fclose(out);

				out = fopen("bench/many-emphasis.md", "w");
				F(i, 1000000) {
					fprintf(out, "*foo* ");
				}

				fclose(out);

				out = fopen("bench/many-fenced-code-blocks.md", "w");
				F(i, 1000000) {
					fprintf(out, "~~~\nfoo\n~~~\n\n\n");
				}

				fclose(out);

				out = fopen("bench/many-links.md", "w");
				// homebrew version of md4c struggles with this one
				//F(i, 1000000) {
				F(i, 100000) {
					fprintf(out, "[a](/url) \n");
				}

				fclose(out);

				out = fopen("bench/many-paragraphs.md", "w");
				F(i, 1000000) {
					fprintf(out, "foo\n\n");
				}

				fclose(out);

				out = fopen("bench/deep-blockquote.md", "w");
				F(i, 10000) {
					fprintf(out, "> ");
				}

				fprintf(out, "foo\n");

				fclose(out);

				out = fopen("bench/shallow-blockquote.md", "w");
				F(i, 1000000) {
					fprintf(out, "> > foo\n\n");
				}

				fclose(out);

				out = fopen("bench/small-file.md", "w");
				F(i, 1) {
					fprintf(out, "# Header #\n\n* foo\n* foo\n\n");
				}

				fclose(out);
			}
			break;

			case 'r': {
				int reps = 5;

				fprintf(stdout, "MultiMarkdown version\n");
				system("../build/multimarkdown --version");

				fprintf(stdout, "\nCompare MMD 7 with different arguments\n");

				F(i, (sizeof(args) / sizeof(args[0]))) {
					struct timespec start, end;
					int64_t total = 0;
					char buffer[256] = {0};
					sprintf(buffer, "../build/multimarkdown %s bench/speed1024.md > /dev/null", args[i]);

					F(k, reps) {
						clock_gettime(CLOCK_MONOTONIC_RAW, &start);
						system(buffer);
						clock_gettime(CLOCK_MONOTONIC_RAW, &end);

						int64_t diff = difftimespec_us(end, start);
						total += diff;
					}

					fprintf(stdout, "%-10s%6.1f msec   (mean over %d runs)\n", args[i], ((double)total / (double)1000 / (double)reps), reps);
				}

				fprintf(stdout, "\nCompare different parsers on same test files.\n(NOTE: MMD has features that other parsers do not.\nThis results in a not insignficant performance hit when parsing headers, for example.\nCompatibility mode turns off (most of) those features.)\n\n");

				system("cmark --version");

				fprintf(stdout, "\nmd4c version\n");
				system("md2html -v");

				fprintf(stdout, "\n");

				F(i, (sizeof(files) / sizeof(files[0]))) {
					// The last file (small-file) will be run for more reps
					if (i == (sizeof(files) / sizeof(files[0])) - 1) {
						reps = 100;
					}

					F(j, (sizeof(commands) / sizeof(commands[0]))) {
						struct timespec start, end;
						int64_t total = 0;
						char buffer[256] = {0};
						sprintf(buffer, "%s %s > /dev/null", commands[j][0], files[i]);

						F(k, reps) {
							clock_gettime(CLOCK_MONOTONIC_RAW, &start);
							system(buffer);
							clock_gettime(CLOCK_MONOTONIC_RAW, &end);

							int64_t diff = difftimespec_us(end, start);
							total += diff;
						}

						fprintf(stdout, "%-10s%-35s%6.1f msec   (mean over %d runs)\n", commands[j][1], files[i], ((double)total / (double)1000 / (double)reps), reps);
					}

					fprintf(stdout, "\n");
				}
			}
			break;
		}
	}


	// Clean up

	return err;
}
