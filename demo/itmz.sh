#!/bin/bash

# Generates each combination of the below
files=(src/shallow.txt src/medium.txt src/deep.txt src/flat.txt src/letter.txt src/integration.txt)

mkdir -p build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	../build/multimarkdown -t itmz "$file" > "build/$name.itmz"
done
