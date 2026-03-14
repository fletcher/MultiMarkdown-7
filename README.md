|            |                           |  
| ---------- | ------------------------- |  
| Title:     | MultiMarkdown v7          |  
| Author:    | Fletcher T. Penney       |  
| Date:      | 2026-03-13 |  
| Copyright: | Copyright © 2024-2026 Fletcher T. Penney.    |  
| Version:   | 7.0.0 |  


# MultiMarkdown 7.0.0 #

*C parser for Markdown with additional features and multiple output formats.*

## Introduction ##

This README has been streamlined.  Please see the User's Guide and Developer's
Guide for additional information.  These can be found 
[here](https://fletcher.github.io/MultiMarkdown-7/).


## Current Status ##

MultiMarkdown v7 supports everything that v6 supported (I think) with one key
exception -- it doesn't currently include OpenDocument (ODT) as an export format.

I am still deciding what I want to do with this.  The ODT format was
relatively easy to generate.  It is a bit verbose compared to HTML or LaTeX,
but not too bad.  But it is still not used as frequently as I would like, and
sadly the Microsoft format is still much more popular.  So I looked into
supporting that as a native export format, and it is a mess(not suprising...)
My hope was that the Word format would be OK to generate, and then I was
going to look into natively generating PowerPoint (in addition to the LaTeX
Beamer support that already exists.)  We'll see.

Otherwise, v7 is ready for the public!


## Installers ##

Currently there are only installers for macOS, which can be found at Github:

<https://github.com/fletcher/MultiMarkdown-7/releases>

I still don't have a great workflow to generate Windows installers for MMD.


## Building MultiMarkdown ##

You can compile MMD from source using CMake:

	make
	cd build
	make
	ctest
	make install

The `ctest` command is optional and verifies that your copy is working
correctly.

The `make install` command is optional and can be used to intall
`multimarkdown` for easy use.

If you want to use Xcode:

	make xcode
	open build-xcode/libMultiMarkdown7.xcodeproj


## Contributing ##

Feature requests, bug reports, etc. can be submitted via the Github
[repository](https://github.com/fletcher/MultiMarkdown-7).


## License ##

	MIT License
	
	Copyright (c) 2024-2026 Fletcher T. Penney
	
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	
	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.


## Components ##

MultiMarkdown uses a few other projects for specific functionality:

* getopt -- Copyright (c) 2002 Todd C. Miller  All rights reserved. (Windows only)

* miniz -- Copyright 2013-2014 RAD Game Tools and Valve Software.  
	Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC  All rights reserved.

* uthash -- Copyright (c) 2003-2022, Troy D. Hanson  All rights reserved.

* yxml -- Copyright (c) 2013-2014 Yoran Heling  All rights reserved.

