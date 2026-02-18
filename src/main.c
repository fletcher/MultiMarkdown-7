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

#if (defined(_WIN32) || defined(__WIN32__))
	#include "getopt.h"
#else
	#include <unistd.h>
#endif

#include "libMultiMarkdown.h"
#include "read_ctx.h"
#include "version.h"
#include "zip.h"


#define F(i,n) for(int i= 0;i<n;i++)


char * filename_with_extension(const char * original, const char * new_extension) {
	char * ext = strrchr(original, '.');
	long len;

	if (ext) {
		len = ext - original;
	} else {
		len = strlen(original);
	}

	char * buffer = malloc((len + strlen(new_extension) + 1) * sizeof(char));

	if (buffer) {
		snprintf(buffer, len + 1, "%s", original);
		snprintf(&buffer[len], strlen(new_extension) + 1, "%s", new_extension);
	}

	return buffer;
}


typedef struct {
	const char *	name;
	const char *	file_extension;
} format;


static format formats[] = {
	[FORMAT_HTML] = { "html", ".html" },
	[FORMAT_EPUB] = { "epub", ".epub" },
	[FORMAT_LATEX] = { "latex", ".tex" },
	[FORMAT_BEAMER] = { "beamer", ".tex" },
	[FORMAT_MEMOIR] = { "memoir", ".tex" },
	[FORMAT_FODT] = { "fodt", ".fodt" },
	[FORMAT_ODT] = { "odt", ".odt" },
	[FORMAT_TEXTBUNDLE] = { "textbundle", ".textbundle" },
	[FORMAT_TEXTPACK] = { "textpack", ".textpack" },
	[FORMAT_OPML] = { "opml", ".opml" },
	[FORMAT_ITMZ] = { "itmz", ".itmz" },
	[FORMAT_MMD] = { "mmd", ".mmdtext" },
	[FORMAT_AST] = { "ast", ".ast" },
	[FORMAT_HASH] = { "hash", ".hash" },
	[FORMAT_DOCX] = { "docx", ".docx" },
};


typedef struct {
	const char * 	abbreviation;
	int				quotes;
} language;


static language languages[] = {
	[LANGUAGE_EN] = { "en", QUOTES_ENGLISH},
	[LANGUAGE_ES] = { "es", QUOTES_SPANISH },
	[LANGUAGE_DE] = { "de", QUOTES_GERMAN },
	[LANGUAGE_FR] = { "fr", QUOTES_FRENCH },
	[LANGUAGE_NL] = { "nl", QUOTES_DUTCH },
	[LANGUAGE_SV] = { "sv", QUOTES_SWEDISH },
	[LANGUAGE_HE] = { "he", QUOTES_ENGLISH },
};


#define kMETAKEYSIZE 1024

int main(int argc, char * const argv[]) {
	if (OBJECT_REPLACEMENT_CHARACTER > 250) {
		// Adding too many token types...
		fprintf(stderr, "CAUTION: OBJECT_REPLACEMENT_CHARACTER nearing 255!! => %d\n", OBJECT_REPLACEMENT_CHARACTER);
		exit(1);
	}

	char action = '\0';
	int option;
	int err = 0;

	char meta_key[kMETAKEYSIZE] = {0};

	uint32_t options = MMD_OPTION_MMD_HEADER;

	char extension[64] = {0};

	// Set offset to 1 if we want an "action" immediately following the program when called
	// e.g.  ./foo bar -x -y -z
	int offset = 0;

	if (argc > 1) {
		if (argv[1][0] != '-' || argv[1][1] == '-') {
			// Skip action or long argument (e.g. `--help`)
			offset = 1;
		}
	}

	custom_seed_rand();

	// Read short options
	while ((option = getopt(argc - offset, &argv[offset], ":cdhbe:l:rst:vyzARx:")) != -1) {
		switch (option) {
			case 'h':
				// help -- display usage
				err = -1;
				break;

			case 'v':
				// version
				fprintf(stdout, "%s\n", LIBMULTIMARKDOWN7_VERSION);
				return 0;

			case 'l':
				// Language/Smart Quotes localization
				// Disable current choices
				options &= (~(MMD_SMART_QUOTE_MASK | MMD_LANGUAGE_MASK));

				F(i, 1 + sizeof(languages) / sizeof(languages[0])) {
					if (i == sizeof(languages) / sizeof(languages[0])) {
						fprintf(stderr, "%s: '%s' is not a valid language\n", argv[0], optarg);
						err = -1;
						break;
					}

					if (languages[i].abbreviation && !strcmp(optarg, languages[i].abbreviation)) {
						options |= (languages[i].quotes << 5);
						options |= (i << 9);
						break;
					}
				}

				break;

			case 'b':
				// Blocks only
				options |= MMD_OPTION_BLOCKS_ONLY;
				break;

			case 'c':
				// Compatibility mode
				options |= MMD_OPTION_COMPATIBILITY;
				break;

			case 'd':
				// Download assets using curl
				options |= MMD_OPTION_DOWNLOAD_ASSETS;
				break;

			case 'r':
				// Enable transclusion
				options |= MMD_OPTION_TRANSCLUDE;
				break;

			case 's':
				// Measure performance stats
				options |= MMD_OPTION_STATS;
				break;

			case 'y':
				// Use random footnote id #
				options |= MMD_OPTION_RANDOM_NOTE_ID;
				break;

			case 'z':
				// Use random footnote id #
				options |= MMD_OPTION_RANDOM_HEADER_ID;
				break;

			case 'A':
				// Accept all proposed CriticMarkup changes
				options &= ~MMD_OPTION_CRITIC_REJECT;
				options |= MMD_OPTION_CRITIC_ACCEPT;
				break;

			case 'R':
				// Reject all proposed CriticMarkup changes
				options &= ~MMD_OPTION_CRITIC_ACCEPT;
				options |= MMD_OPTION_CRITIC_REJECT;
				break;

			case 't':
				// output format (FORMAT_HTML is the default)
				// Disable current format
				options &= (~MMD_OUT_FORMAT_MASK);

				F(i, 1 + sizeof(formats) / sizeof(formats[0])) {
					if (i == sizeof(formats) / sizeof(formats[0])) {
						fprintf(stderr, "%s: '%s' is not a valid output format\n", argv[0], optarg);
						err = -1;
						break;
					}

					if (formats[i].name && !strcmp(optarg, formats[i].name)) {
						options |= i;
						break;
					}
				}

				break;

			case 'x':
				// Customize extension for batch mode
				strncpy(extension, optarg, 63);
				break;

			case 'e':
				// Metadata key to exxtract
				strncpy(meta_key, optarg, kMETAKEYSIZE - 1);
				break;

			case ':':
				// required argument is missing
				fprintf(stdout, "%s: option needs value -- %c\n", argv[0], optopt);
				err = 1;
				break;

			case '?':
				// option is not recognized
				fprintf(stderr, "%s: option not recognized -- %c\n", argv[0], optopt);
				err = 1;
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
		} else if (strcmp(argv[1], "help") == 0) {
			// help -- display usage
			err = -1;
		} else if (strcmp(argv[1], "--version") == 0) {
			fprintf(stdout, "%s\n", LIBMULTIMARKDOWN7_VERSION);
		} else if (strcmp(argv[1], "parse") == 0) {
			action = 'p';
		} else if (strcmp(argv[1], "batch") == 0) {
			action = 'b';
		} else if (strcmp(argv[1], "meta") == 0) {
			action = 'm';
		} else if (strcmp(argv[1], "ast") == 0) {
			action = 'p';
			options &= (~MMD_OUT_FORMAT_MASK);
			options |= FORMAT_AST;
		} else if (strcmp(argv[1], "hash") == 0) {
			action = 'p';
			options &= (~MMD_OUT_FORMAT_MASK);
			options |= FORMAT_HASH;
		} else {
			fprintf(stderr, "%s: action not recognized -- %s\n", argv[0], argv[1]);
			err = 1;
		}
	}


	if (err) {
		// Error
		fprintf(stderr, "\nMultiMarkdown %s -- %s\n\n", LIBMULTIMARKDOWN7_VERSION, LIBMULTIMARKDOWN7_COPYRIGHT);

		fprintf(stderr, "usage: %s [--help] {ast|batch|hash|meta|parse} [-b] [-h] [-r] [-s] [-e META_KEY] [-l LANGUAGE] [-t FORMAT]\n", argv[0]);

		fprintf(stderr, "\nActions:\n");
		fprintf(stderr, "\tast\t\tDisplay abstract syntax tree for the document\n");
		fprintf(stderr, "\tbatch\t\tProcess each file individually and write a file with default extension\n");
		fprintf(stderr, "\thash\t\tDisplay abstract syntax tree along with hash values for each node\n");
		fprintf(stderr, "\tmeta\t\tDisplay metadata from the parsed document\n");
		fprintf(stderr, "\tparse\t\tParse (each) document and export to the desired format\n");

		fprintf(stderr, "\nOptions:\n");
		fprintf(stderr, "\t-h, --help\tShow this help\n");
		fprintf(stderr, "\t-c\t\tMarkdown compatibility mode\n");
		fprintf(stderr, "\t-d\t\tDownload assets (images, CSS) for inclusion in package formats\n");
		fprintf(stderr, "\t-r\t\tEnable file transclusion (\"recursive\")\n");
		fprintf(stderr, "\t-A\t\tAccept all CriticMarkup changes\n");
		fprintf(stderr, "\t-R\t\tReject all CriticMarkup changes\n");
		fprintf(stderr, "\t-b\t\tLimit parsing to block level only\n");
		fprintf(stderr, "\t-s\t\tLog some processing time statistics\n");
		fprintf(stderr, "\t-e META_KEY\tSpecify metadata key to extract\n");
		fprintf(stderr, "\t-l LANGUAGE\tSpecify language for smart quotes and default markup [en|es|de|fr|nl|sv|he]\n");
		fprintf(stderr, "\t-t FORMAT\tSpecify output format [html|mmd]\n");

		fprintf(stderr, "\nuthash -- Copyright (c) 2003-2022, Troy D. Hanson  https://troydhanson.github.io/uthash/  All rights reserved.\n\n");
	} else {
		// Proceed
		switch (action) {
			case 'h': {
				// Deprecated
				// Parse the specified document(s) or input on stdin and export the AST with hash values
				mmd_node * n;

				if (optind + offset < argc) {
					for (optind += offset; optind < argc; optind++) {
						mmd_hash_filename(argv[optind], stdout, options);
					}
				} else {
					mmd_hash_file(stdin, stdout, options);
				}
			}

			break;

			case 'b':

				// Parse the specified document(s) and export to individual file(s)
				if (optind + offset < argc) {
					for (optind += offset; optind < argc; optind++) {
						char * new_file;

						if (extension[0]) {
							new_file = filename_with_extension(argv[optind], extension);
						} else {
							new_file = filename_with_extension(argv[optind], formats[MMD_OUT_FORMAT_FROM_OPTS(options)].file_extension);
						}

						if (MMD_OUT_FORMAT_FROM_OPTS(options) == FORMAT_TEXTBUNDLE) {
							size_t len;
							char * data = mmd_process_filename_to_str(argv[optind], &len, options, NULL);

							zip_binary_extract_to_path(data, len, new_file);
							free(data);
						} else {
							FILE * out = fopen(new_file, "w");

							if (out) {
								mmd_process_filename(argv[optind], out, options, NULL);

								fclose(out);
							}
						}

						free(new_file);
					}
				} else {
					fprintf(stderr, "Cannot batch from stdin.\n");
				}

				break;

			case 'p':

				// Parse the specified document(s) or input on stdin and export on stdout
				if (optind + offset < argc) {
					for (optind += offset; optind < argc; optind++) {
						mmd_process_filename(argv[optind], stdout, options, NULL);
					}
				} else {
					char buf[1024] = {0};
					char * wd = getcwd(buf, 1024);

					mmd_process_file(stdin, stdout, options, wd, NULL);

					free(wd);
				}

				break;

			case 'm':

				// Parse the specified document(s) or input on stdin and export the metadata keys on stdout
				if (optind + offset < argc) {
					for (optind += offset; optind < argc; optind++) {
						read_ctx *r = mmd_metadata_filename(argv[optind], options);

						meta * m, * m_tmp;

						if (meta_key[0] == '\0') {
							HASH_ITER(hh, r->meta_hash, m, m_tmp) {
								fprintf(stdout, "%s\n", m->key);
							}
						} else {
							m = read_ctx_get_meta(r, meta_key);

							if (m) {
								fprintf(stdout, "%s\n", m->value);
							}
						}

						read_ctx_free(r);
					}
				} else {
					read_ctx * r = mmd_metadata_file(stdin, options);
					meta * m, * m_tmp;

					if (meta_key[0] == '\0') {
						HASH_ITER(hh, r->meta_hash, m, m_tmp) {
							fprintf(stdout, "'%s'\n", m->key);
						}
					} else {
						m = read_ctx_get_meta(r, meta_key);

						if (m) {
							fprintf(stdout, "%s\n", m->value);
						}
					}

					read_ctx_free(r);
				}

				break;

			case 'a':

				// Deprecated
				// Output the AST for the specified document(s) or input on stdin
				if (optind + offset < argc) {
					for (optind += offset; optind < argc; optind++) {
						mmd_ast_filename(argv[optind], stdout, options);
					}
				} else {
					mmd_ast_file(stdin, stdout, options);
				}

				break;
		}
	}


	// Clean up

	return err;
}

