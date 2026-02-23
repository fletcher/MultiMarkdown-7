#!/bin/bash

command="${1}"
args="${2}"
dir=${3}
ext="${4}"

passed=0
failed=0

for file in "$dir"/*.text; do
	# Look for matching file with desired extension
	target="${file%.*}.$ext"

	if [[ -f "$target" ]]; then
		name=$(basename -s .$ext "$target")

		# Process text file
		output=$("$command" $args "$file")
		answer=$(<"$target")

		# Compare results
		if [[ "$output" == "$answer" ]]; then
			echo "$name => OK"
			((passed++))
		else
			echo "$name => failed"
			((failed++))
			echo "$output" | diff "$target" -
		fi
	fi
done

echo "$passed passed; $failed failed."

exit $failed
