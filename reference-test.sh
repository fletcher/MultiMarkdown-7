#!/bin/sh

cd build
make

./generate_test_reference > ref.text


cmark ref.text | tidy -asxhtml > ref-cmark.html
./multimarkdown -C ref.text > ref-mmd-c.html
opendiff ref-cmark.html ref-mmd-c.html


./multimarkdown -C ref.text > ref-mmd.html
multimarkdown6 -f ref.text > ref-mmd6.html
opendiff ref-mmd6.html ref-mmd.html

# open ref-mmd6.html ref-mmd.html
