#!/bin/bash

# Generates each combination of the below
files=(src/integration.txt ../tests/MMD7Tests/Integrated.text)
packages=(mmd7-core)
options=("")


mkdir -p build
cp assets/test-image.png build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	level = "\nbaseheaderlevel: 1"

	if [[ "$name" == "integration" ]]; then
		level="\nbaseheaderlevel: 2"
	fi

	for package in "${packages[@]}"; do
		echo -e "latexclass: [showtoc]report\nlatexpackage: $option$package$level" | cat - "$file" | ../build/multimarkdown -E -t latex > build/$name-report-$package-$option.tex
	done
done

cd build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		latexmk -c "$name-report-$package-$option.tex"
		pdflatex "$name-report-$package-$option.tex"
		pdflatex "$name-report-$package-$option.tex"
		pdflatex "$name-report-$package-$option.tex"
		pdflatex "$name-report-$package-$option.tex"
		open "$name-report-$package-$option.pdf"
	done
done
