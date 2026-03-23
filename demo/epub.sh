#!/bin/bash

# Generates each combination of the below
files=(src/letter.txt src/medium.txt src/deep.txt src/flat.txt src/integration.txt)

mkdir -p build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	../build/multimarkdown -E -D -t epub "$file" > "build/$name.epub"
done
