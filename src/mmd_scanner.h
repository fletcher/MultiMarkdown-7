/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_scanner.h

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


#ifndef MMD_SCANNER_LIBMULTIMARKDOWN7_H
#define MMD_SCANNER_LIBMULTIMARKDOWN7_H


/// Re2c scanner data -- this structure is used by the re2c
/// scanner to track progress and offsets within the source
/// string.
struct Scanner {
	const char *	text;		//!< Start of text to be scanned
	const char *	stop;		//!< End of text to be scanned
	const char *	start;		//!< Start of current token
	const char *	cur;		//!< Character currently being matched
	const char *	ptr;		//!< Used for backtracking by re2c
	const char *	ctx;
	unsigned char	curType;	//!< Type of current token
	int				depth;

	int				allow_meta;	//!< Is metadata allowed here?

	const char *	c_start;
	const char *	c_end;
};

typedef struct Scanner Scanner;


// Basic scanner struct

#define YYCTYPE		unsigned char
#define YYCURSOR	s->cur
#define YYMARKER	s->ptr
#define YYCTXMARKER	s->ctx
#define YYLIMIT		s->stop
#define YYDEBUG(state, symbol) fprintf(stdout, "%d => %c\n", state, symbol)


/// Build a scanner for a given text
Scanner mmd_scanner(const char * text, size_t len);

#endif
