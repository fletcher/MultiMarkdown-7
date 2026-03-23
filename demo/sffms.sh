#!/bin/bash

# Generates each combination of the below
files=(src/letter.txt)
packages=(mmd7-core)
options=("")


mkdir -p build
cp assets/test-image.png build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		echo -e "latexclass: sffms\nlatexpackage: $option$package" | cat - "$file" | ../build/multimarkdown -E -t latex > build/$name-sffms-$package-$option.tex
	done
done

cd build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		latexmk -c "$name-sffms-$package-$option.tex"
		pdflatex "$name-sffms-$package-$option.tex"
		pdflatex "$name-sffms-$package-$option.tex"
		pdflatex "$name-sffms-$package-$option.tex"
		pdflatex "$name-sffms-$package-$option.tex"
		open "$name-sffms-$package-$option.pdf"
	done
done
