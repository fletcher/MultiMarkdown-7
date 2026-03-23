#!/bin/bash

# Generates each combination of the below
files=(src/medium.txt src/deep.txt src/flat.txt src/integration.txt)

mkdir -p build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	../build/multimarkdown -t html "$file" > "build/$name.html"
done
