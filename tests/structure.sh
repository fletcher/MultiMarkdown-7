#!/bin/bash

# Generate every combination of 4 of these lines to test for edge cases in parsing 
# blocks of various structures

# It can be useful to compare the HTML output of MMD 7 with MMD 6 and CommonMark
# to ensure that any differences are intentional.


lines=("# Header 1 #" "Plain text." "* Bulleted item" "1. Enumerated item" "=========" "---------" "	Indented line" "> Blockquote line" "---" "")


for index1 in ${!lines[@]}; do
	for index2 in ${!lines[@]}; do
		for index3 in ${!lines[@]}; do
			for index4 in ${!lines[@]}; do
				for index5 in ${!lines[@]}; do
					echo "$index1-$index2-$index3-$index4-$index5"
					echo ""
					echo "${lines[index1]}"
					echo "${lines[index2]}"
					echo "${lines[index3]}"
					echo "${lines[index4]}"
					echo "${lines[index5]}"
					echo ""
					echo ""
				done
			done
		done
	done
done
