/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file mmd_core.h

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


#ifndef MMD_CORE_libMultiMarkdown7_H
#define MMD_CORE_libMultiMarkdown7_H


void mmd_process_buffer(text_buffer * source_buffer, text_buffer * out_buffer, uint32_t options, const char * search_path, const char * source_path);
char * mmd_process_buffer_to_str(text_buffer * source_buffer, size_t * out_len, uint32_t options, const char * search_path, const char * source_path);
mmd_node * mmd_parse_buffer(text_buffer * buffer, read_ctx * c, uint32_t options);
void mmd_ast_buffer(text_buffer * buffer, FILE *out, uint32_t options);
void mmd_hash_buffer(text_buffer * buffer, FILE *out, uint32_t options);
read_ctx * mmd_metadata_buffer(text_buffer * buffer, uint32_t options);
read_ctx * mmd_tags_buffer(text_buffer * buffer, uint32_t options);


#endif
