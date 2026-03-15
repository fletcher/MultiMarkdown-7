#!/bin/bash

# Update source files from develop branch
git checkout develop .

# Update HTML
../build/multimarkdown batch -r dev.md history.md user.md
