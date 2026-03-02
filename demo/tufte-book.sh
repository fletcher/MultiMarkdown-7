#!/bin/bash

# Generates each combination of the below
files=(src/letter.mmd src/medium.mmd ../tests/MMD7Tests/Integrated.text)
packages=(mmd7-core)
options=("")


mkdir -p build
cp assets/test-image.png build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		echo -e "latexclass: tufte-book\nlatexpackage: $option$package\nbaseheaderlevel: 1" | cat - "$file" | ../build/multimarkdown -E -t latex > build/$name-tufte-book-$package-$option.tex
	done
done

cd build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		latexmk -c "$name-tufte-book-$package-$option.tex"
		pdflatex "$name-tufte-book-$package-$option.tex"
		pdflatex "$name-tufte-book-$package-$option.tex"
		pdflatex "$name-tufte-book-$package-$option.tex"
		pdflatex "$name-tufte-book-$package-$option.tex"
		open "$name-tufte-book-$package-$option.pdf"
	done
done
