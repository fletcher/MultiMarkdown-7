#!/bin/bash

# Generates each combination of the below
files=(src/letter.txt ../tests/MMD7Tests/Integrated.text)
# files=(src/letter.txt)
packages=(mmd7-letterhead)
options=("" "[printed]")
# options=("")
addresses=("" "My Street \\\\\\\\ My City, USA  12345")
# addresses=("")
returnaddresses=("" "My Other Street \\\\\\\\ My Other City, USA  12345")
# returnaddresses=("")


mkdir -p build
cp assets/test-image.png build

for file in "${files[@]}"; do
	base=$(basename "${file}")
	name=${base%.*}

	for package in "${packages[@]}"; do
		for option in "${options[@]}"; do
			for address in "${addresses[@]}"; do
				if [[ "$address" == "" ]]; then
					addy=""
					flags=""
				else
					addy="\naddress: $address"
					flags="a"
				fi
				for return in "${returnaddresses[@]}"; do
					if [[ "$return" == "" ]]; then
						raddy=""
					else
						raddy="\nreturn address: $return"
						flags+="r"
					fi
					echo -e "latexclass: letter\nlatexpackage: $option$package$addy$raddy" | cat - "$file" | ../build/multimarkdown -E -t latex > build/$name-$package-$option-$flags.tex
				done
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
			for address in "${addresses[@]}"; do
				if [[ "$address" == "" ]]; then
					flags=""
				else
					flags="a"
				fi
				for return in "${returnaddresses[@]}"; do
					if [[ "$return" != "" ]]; then
						flags+="r"
					fi
					latexmk -c "$name-$package-$option-$flags.tex"
					xelatex "$name-$package-$option-$flags.tex"
					xelatex "$name-$package-$option-$flags.tex"
					open "$name-$package-$option-$flags.pdf"
				done
			done
		done
	done
done
