#!/bin/bash

# Generates each combination of the below
files=(src/medium.mmd src/deep.mmd src/flat.mmd)

mkdir -p build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	../build/multimarkdown -t html "$file" > "build/$name.html"
done
