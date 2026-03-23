#!/bin/bash

# Generates each combination of the below
files=(src/letter.txt src/shallow.txt src/flat.txt src/integration.txt)
packages=(mmd7-core)
options=("")


mkdir -p build
cp assets/test-image.png build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		echo -e "latexclass: tufte-handout\nlatexpackage: $option$package\nbaseheaderlevel: 3" | cat - "$file" | ../build/multimarkdown -E -t latex > build/$name-tufte-handout-$package-$option.tex
	done
done

cd build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		latexmk -c "$name-tufte-handout-$package-$option.tex"
		pdflatex "$name-tufte-handout-$package-$option.tex"
		pdflatex "$name-tufte-handout-$package-$option.tex"
		pdflatex "$name-tufte-handout-$package-$option.tex"
		pdflatex "$name-tufte-handout-$package-$option.tex"
		open "$name-tufte-handout-$package-$option.pdf"
	done
done
