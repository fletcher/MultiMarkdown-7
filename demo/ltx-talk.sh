#!/bin/bash

# Generates each combination of the below
files=(src/ltx-talk.txt)
packages=(mmd7-stage-talk mmd7-ltx-talk)
# packages=(mmd7-ltx-talk)
options=("[showtoc,showthanks,showreferences]")


mkdir -p build
cp assets/test-image.png build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		for option in "${options[@]}"; do
			# There seem to be some discrepancies in how ltx-talk with and without stage-talk handles frame titles, e.g.:
			#	\begin{frame}{Stage-talk title here}
			# vs
			#	\begin{frame} \frametitle{ltx-talk title here}
				if [[ "$package" == "mmd7-stage-talk" ]]; then
					echo -e "latexclass: ltx-talk\nlatexpackage: $option$package" | cat - "$file" | ../build/multimarkdown -E -t ltx-talk > "build/$name-$package.tex"
				else
					echo -e "latexclass: ltx-talk\nlatexpackage: $option$package" | cat - "$file" | ../build/multimarkdown -E -t beamer > "build/$name-$package.tex"
				fi
		done
	done
done

cd build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		for option in "${options[@]}"; do
			latexmk -c "$name-$package.tex"
			lualatex "$name-$package.tex"
			lualatex "$name-$package.tex"
			lualatex "$name-$package.tex"
			lualatex "$name-$package.tex"
			open "$name-$package.pdf"
		done
	done
done
