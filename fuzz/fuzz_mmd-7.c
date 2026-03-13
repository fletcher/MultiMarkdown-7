#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "libMultiMarkdown7.h"
#include "text_buffer.h"
#include "read_ctx.h"

uint32_t options[] = {
	0,
	MMD_OPTION_COMPATIBILITY,
	FORMAT_LATEX,
	FORMAT_LATEX | MMD_OPTION_COMPATIBILITY,
	FORMAT_MMD,
	FORMAT_MMD | MMD_OPTION_COMPATIBILITY,
	// FORMAT_EPUB,
	// FORMAT_EPUB | MMD_OPTION_COMPATIBILITY,
	// FORMAT_DOCX
};

#define F(i,n) for(int i= 0;i<n;i++)


// NOTES:
//	Test EPUB format is *slow* by comparison, so it is generally disabled.


int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) {
	// We need to null pad input since we can't guarantee that
	// text_buffer * buf = text_buffer_new(size + 1);
	// text_buffer_append_text(buf, (const char *) data, size);
	// buf->text[size] = '\0';

	// Actually -- don't do this. If it needs to be done, do it in MMD.
	// But ideally it should not need to be done.

	if (1) {
		// Test each set of options by writing straight to /dev/null
		F(i, sizeof(options) / sizeof(options[0])) {
			FILE *out = fopen("/dev/null", "w");
			mmd_process_str_len((const char *) data, size, out, options[i], NULL, NULL);
			fclose(out);
		}
	}

	if (1) {
		// Test each set of options by writing straight to /dev/null
		F(i, sizeof(options) / sizeof(options[0])) {
			FILE *out = fopen("/dev/null", "w");
			mmd_ast_str_len((const char *) data, size, out, options[i]);
			fclose(out);
		}
	}

	if (1) {
		// Test each set of options by writing straight to /dev/null
		F(i, sizeof(options) / sizeof(options[0])) {
			FILE *out = fopen("/dev/null", "w");
			mmd_hash_str_len((const char *) data, size, out, options[i]);
			fclose(out);
		}
	}

	if (1) {
		// Test each set of options by writing to a string, and then freeing the string
		// This is probably redundant since what we are really interested in is common
		// between all of the API methods...
		F(i, sizeof(options) / sizeof(options[0])) {
			size_t out_len;
			char * out = mmd_process_str_len_to_str((const char *) data, size, &out_len, options[i], NULL, NULL);
			free(out);
		}
	}

	if (1) {
		F(i, sizeof(options) / sizeof(options[0])) {
			read_ctx * r = read_ctx_new(options[i]);
			mmd_node * n = mmd_parse_str_len((const char *) data, size, r, options[i]);
			read_ctx_free(r);
			mmd_node_tree_free(n);
		}
	}

	return 0;
}
