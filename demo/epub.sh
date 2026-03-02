#!/bin/bash

# Generates each combination of the below
files=(src/letter.mmd src/medium.mmd src/deep.mmd src/flat.mmd src/integration.mmd)

mkdir -p build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	../build/multimarkdown -E -D -t epub "$file" > "build/$name.epub"
done
