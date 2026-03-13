|            |                           |  
| ---------- | ------------------------- |  
| Title:     | libMultiMarkdown7        |  
| Author:    | Fletcher T. Penney       |  
| Date:      | 2026-02-10 |  
| Copyright: | Copyright © 2024-2026 Fletcher T. Penney.    |  
| Version:   | 7.0.0-beta.1 |  


# MultiMarkdown 7.0.0-beta.1 #

*Lightweight markup processor to produce HTML, LaTeX, and more.*


## About ##

Why a new version of MultiMarkdown?  Briefly, I wanted to:

*	Improve performance -- MMD v6 was a big improvement over prior versions,
	but time goes on and performance was no longer "up to snuff"

*	Clean up the code -- v6 was a (almost) complete rewrite from v5, and
	there was a lot of "learning while building" during development that
 	lead to sloppy code in some places

*	Improve the API -- the v6 API made sense to me at the time during
	development, but didn't really fit with the way MMD would be called
	as a library

*	Simplify -- simple code is almost always easier to maintain and offers
	better performance....  There was a lot of room to simplify!

*	Improve the syntax and output -- during development and testing I was
	able to continue to make small fixes to various edge cases.  But
	generally the syntax for MultiMarkdown 7 is the same as MultiMarkdown 6.


## Current Status (2025-09-10) ##

*	HTML support should be complete.  I've had to update the HTML test suite
	from v6, but all of the tests are passed.  This *should* represent
	intentional changes to the HTML output, but some inadvertent changes may
	have snuck through.  I will add support for other formats as time goes on
	and as I become more confident that the core engine is complete and
	correct.

*	HTML compatibility mode passes the test suite. Compatibility mode turns
	off functionality that is not part of the core Markdown syntax (e.g.
	footnotes, citations, fenced code blocks, etc.).

*	LaTeX support should be complete.  The intent is that changes between
	v6 and v7 in HTML are mirrored in LaTeX as well.  There are some other
	small adjustments that should not affect the rendered document but
	simplify the LaTeX source slightly.

*	I have included support for using vagrant to run fuzz testing using
	[libFuzzer](https://llvm.org/docs/LibFuzzer.html).  I have used that to
	find (and fix) many bugs, making the parser more resilient against
	"unusually" formed input that I would not have thought to test.  I did
	this with v6 as well, but now it is built directly into the development
	repository. This also tests for memory leaks in the core engine. In addition
	to "bugs", this can also identify pathologic input that needs to be handled.

*	Performance is good -- depending on the test content, it is *significantly*
	faster than MMD v6.  It is usually faster than CommonMark (v 0.31.1). And
	sometimes it is almost getting close to md4c.  Things to remember:

	*	MultiMarkdown has more features than CommonMark and md4c. Some of these
		will necessarily cause a performance hit when compared to not using
		those features (such as the more complex output for `# Headers #`).
		Compatibility mode turns off most of those features, but in a few cases
		there will still be a slight performance cost even in compatibility
		mode.

	*	MultiMarkdown has to be capable of returning an AST in order to be 
		used in some situations (e.g. as a syntax highlighter for MultiMarkdown
		Composer.)  This means it will necessarily be slower than md4c in all
		but the most trivial of test cases.  That said, I include md4c in the 
		benchmark testing as something of a "lower bound" to shoot for.

	*	The benchmark testing (in /dev) shows comparisons between MMD 6, MMD 7,
		CommonMark, and md4c (if you have those installed).  Additionally, it
		compares HTML, Compatibility, and LaTeX output in MMD 7. (NOTE: md4c
		installed via homebrew chokes on the many-links test file.  I don't
		know why.  Building md4c yourself from source repo works fine.)

*	The API is cleaner.  I still need some testing and feedback on this, but
	I am generally happy so far.  I've tried to make `libMultiMarkdown7.h`
	more self-explanatory and more complete.  That said, I have not used
	MMD 7 inside a "real" project yet, so this may change.

*	The command line interface is different.  The code is much cleaner, but
	I am likely to make some changes here as I get out of the development
	phase and more into the "using it" phase.

*	The code is much cleaner.  Still room to improve, of course!

*	So far I have been happy with the changes to edge cases in the output. I
	started by trying to match v6 in the test suite, but a few instances arose
	where it was clear that the old "correct" could be improved upon.  More
	testing will help find instances where I should make additional fixes.

## Building ##

To build MultiMarkdown, use CMake:

```
make
cd build
make
```

Xcode users can use:

```
make xcode
open build-xcode/libMultiMarkdown7.xcodeproj
```


## Testing ##

```
make
cd build
make
ctest
```

You can use `ctest -V` in order to see more information on failed tests.


## Fuzz Testing ##

```
cd fuzz
make
cd build
make
./fuzz_mmd-7
```

This does not work on macOS.  You can use Vagrant, if installed, in order to
perform fuzz testing on macOS.  Please ensure you know how to properly use
Vagrant.


```
vagrant up
vagrant ssh
cd /vagrant
cd fuzz
make
cd build
make
./fuzz_mmd-7
```


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


uthash License

	Copyright (c) 2003-2022, Troy D. Hanson  https://troydhanson.github.io/uthash/
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
	IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
	PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
	OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
	EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
