#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "libMultiMarkdown.h"
#include "text_buffer.h"
#include "read_ctx.h"

uint32_t options[] = {
	0,
	MMD_OPTION_COMPATIBILITY,
	FORMAT_LATEX,
	FORMAT_LATEX | MMD_OPTION_COMPATIBILITY
};

#define F(i,n) for(int i= 0;i<n;i++)


int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) {
	// We need to null pad input since we can't guarantee that
	// text_buffer * buf = text_buffer_new(size + 1);
	// text_buffer_append_text(buf, (const char *) data, size);
	// buf->text[size] = '\0';

	// Actually -- don't do this. If it needs to be done, do it in MMD.
	// But ideally it should not need to be done.

	if (0) {
		// Test each set of options by writing straight to /dev/null
		F(i, sizeof(options) / sizeof(options[0])) {
			FILE *out = fopen("/dev/null", "w");
			mmd_process_str_len((const char *) data, size, out, options[i], NULL, NULL);
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

	if (0) {
		// Need to figure out best strategy for memory management of nodes/vector/pool
		// when calling this function.
		// Otherwise this immediately registers a leak
		// Though, I'm not sure there is really much point
		read_ctx * r = read_ctx_new(0);
		mmd_node * n = mmd_parse_str_len((const char *) data, size, r, 0);
		read_ctx_free(r);
		mmd_node_tree_free(n);
	}

	return 0;
}
