/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_tokenizer.h

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


#ifndef MMD_TOKENIZER_LIBMULTIMARKDOWN7_H
#define MMD_TOKENIZER_LIBMULTIMARKDOWN7_H


typedef struct mmd_tokenizer mmd_tokenizer;

/// Create new tokenizer for a given block
mmd_tokenizer * mmd_tokenizer_new(const char * text, mmd_node * block, read_ctx * c, mmd_node_pool * p, uint32_t options);

void mmd_tokenizer_free(mmd_tokenizer * z);

/// Type for next token from tokenizer
unsigned char mmd_tokenizer_node_type(mmd_tokenizer * z);


/// Get current token and scan the next
mmd_node * mmd_tokenizer_accept_token(mmd_tokenizer * z, mmd_node_pool * p, uint32_t options);


#endif
