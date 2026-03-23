#!/bin/bash

# Generates each combination of the below
files=(src/medium.txt src/deep.txt src/flat.txt src/integration.txt)
# files=(src/medium.txt)
packages=(mmd7-beamer)
options=("")
themes=(default durham keynote-gradient)


mkdir -p build
cp assets/test-image.png build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		for option in "${options[@]}"; do
			for theme in "${themes[@]}"; do
				echo -e "latexclass: [ignorenonframetext,12pt,aspectratio=169]beamer\nlatexpackage: $option$package\ntheme: $theme" | cat - "$file" | ../build/multimarkdown -E -t beamer > "build/$name-$package-$option-$theme.tex"
			done
		done
	done
done

cd build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		for option in "${options[@]}"; do
			for theme in "${themes[@]}"; do
				latexmk -c "$name-$package-$option-$theme.tex"
				pdflatex "$name-$package-$option-$theme.tex"
				pdflatex "$name-$package-$option-$theme.tex"
				pdflatex "$name-$package-$option-$theme.tex"
				pdflatex "$name-$package-$option-$theme.tex"
				open "$name-$package-$option-$theme.pdf"
			done
		done
	done
done
