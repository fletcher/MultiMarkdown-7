#!/bin/bash

# Generates each combination of the below
files=(src/letter.txt src/integration.txt ../tests/MMD7Tests/Integrated.text)
# files=(src/integration.txt)


classes=("article" "[oneside,article]memoir" "tufte-handout")
options=("")


mkdir -p build
cp assets/test-image.png build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	# echo -e "latexconfig: letterhead-musc" | cat - "$file" | ../build/multimarkdown -t latex > build/$name-musc.tex

	for class in "${classes[@]}"; do
		level=""

		if [[ "$name" == "integration" ]]; then
			level="\nbaseheaderlevel: 3"
		fi

		if [[ "$class" == "article" ]]; then
			level="\nbaseheaderlevel: 3"
		fi

		for option in "${options[@]}"; do
			echo -e "latexclass: $class\nlatexpackage: mmd7-article$level" | cat - "$file" | ../build/multimarkdown -E -t latex > build/$name-$class-$option.tex
		done
	done
done

cd build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	# latexmk -c "$name-musc.tex"
	# xelatex "$name-musc.tex"
	# xelatex "$name-musc.tex"
	# open "$name-musc.pdf"

	for class in "${classes[@]}"; do
		for option in "${options[@]}"; do
			latexmk -c "$name-$class-$option.tex"
			pdflatex "$name-$class-$option.tex"
			pdflatex "$name-$class-$option.tex"
			open "$name-$class-$option.pdf"
		done
	done
done
