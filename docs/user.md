title:	MultiMarkdown User's Guide
author:	Fletcher T. Penney
version:	7.0.0
revised:	2026-03-12
baseheaderlevel:	1
css:	css/Classless.min.css
zhtmlheader:	<style>.TOC {position: absolute; left: 100px; width:250px; }</style>
htmlheader:	<style>main {display: grid; grid-template-columns: 15em 1fr; margine: 0 auto; } main > nav {position: sticky; align-self: start; top: 2rem; animation-timeline: view();}</style>
mmdheader: {{header.md}}
mmdfooter: {{footer.md}}

## Introduction ##
### What is Markdown? ###

[Markdown] is a program and a syntax by John Gruber that allows you to easily
convert plain text into HTML suitable for using on a web page.

> The overriding design  goal for Markdown's formatting syntax is  to make it as
> readable as possible. The idea is that a Markdown-formatted document should be
> publishable as-is,  as plain text,  without looking  like it's been  marked up
> with  tags  or  formatting  instructions. While  Markdown's  syntax  has  been
> influenced by several existing text-to-HTML filters, the single biggest source
> of  inspiration for  Markdown's  syntax is  the format  of  plain text  email.
> [][#Gruber]


[#Gruber]: John Gruber.  Daring Fireball: Markdown. [Cited January 2006]. Available from <http://daringfireball.net/projects/markdown/>.

### What is MultiMarkdown? ###

[MultiMarkdown] is similarly two things -- a superset of the functionality
contained within the Markdown syntax and a program that converts that syntax
into multiple output formats, including HTML, EPUB, LaTeX, OPML, and others.


### What's new in MultiMarkdown v7? ###

MMD v7 accomplishes a few things:

* Cleaned up code.  MMD v6 is 10 years old and it was time for something
  better.

* Improved performance.

* Improved API to make it easier for developers to incorporate MMD in their
  own projects.

* Improved accuracy and "correctness".  There are changes to some edge cases
  in order to ensure consistency when processing different documents using
  MMD, and when comparing MMD output to other key Markdown variants.


## Installation ##
### Where to Obtain MultiMarkdown ###

The source code for MMD is available on [Github][repo].

### How to Compile MultiMarkdown ###

MMD uses [Cmake][cmake] as the underlying build system.  Compiling
proceeds in two stages:

    make
    cd build
    make

You can test to ensure that everything works correctly:

    ctest


### How to Install MultiMarkdown ###

TODO: Build installers automatically

## Usage ##
### Basic Usage ###

First, ensure that you have properly installed MMD:

    multimarkdown --version

You can get help from the program itself:

    multimarkdown --help

Convert a MMD text file into HTML:

    multimarkdown source.txt > output.html

Convert source text from stdin into LaTeX:

    cat source.txt | multimarkdown -t latex > output.tex


Check the program's help for more information.


### Batch Mode ###

MMD can parse multiple files in batch mode, automatically generating output
files with the same filename but different extensions.

    multimarkdown batch *.txt


### Prepend Metadata ###

At times it can be useful to add information to a file at processing time,
rather than storing it in the the file.  Since metadata is always at the top
of the file, it can be prepended before passing the source to MMD on stdin.

    echo -e "latexclass: article\nlatexpackage: mmd7-article" | cat - file.txt | multimarkdown -t latex > file.tex

This can be especially useful with scripting workflows.

Since metadata is stored on a "first come, first served" basis, any metadata
prepended in this fashion will take precedence over metadata that is already
in the file.


### Other Options ###

MMD has a few other optional features:

* Compatibility mode (`-c`) -- disables most functionality that was not in the original `Markdown.pl` script.

* Smart quotes -- MMD generates more typographically correct punctuation, such
  as quotation marks. The `quotes language` metadata can change this to match
  English, Dutch, French, German (or German Guillemets), Spanish, or Swedish
  by using the 2-letter country code.  This is disabled in compatibility
  mode.

* Random header IDs (`-z`) -- Normally a header is assigned an id based on the
  title of the header.  This generates a pseudo-random identifier instead.
  Useful if you have large number of headers with the same names.

* Random footnote IDs (`-y`) -- use pseudo-random identifiers for footnotes.
  Useful when combining multiple short documents (e.g. on a blog), where each
  might have a footnote labeled `1`.

* Statistics reporting (`-s`) -- reports how long processing took.  Useful for
  benchmarking or just curiosity.

* Block only parsing (`-b`) -- stops parsing at the block level.  In other
  words, it will determine that a paragraph exists, but will not parse the
  content inside the paragraph.  This may be useful if you just want to see
  the overall structure of a document, or if you are using MMD as a library
  in your own application.


## Output Formats ##
### Complete Documents vs Snippets ###

By default MMD generates "snippets" in several output formats.  For example,
with HTML this means that MMD generates everything that goes inside the
`<body>` tag.  Conversely, a "complete" document includes everything that
should be in place before and after the core content.

By default, MMD will generate a snippet, unless the document contains
metadata.  You can use the `-C` option to force a complete document, or the
`-S` option to force a snippet.


### Embedding ###

MMD has a few options that help with certain more complex needs.

*Embedding* (`-E`) atttempts to embed certain files within the output file
itself.  Currently this works with HTML, and will embed CSS files and image
files within the HTML that is generated.  This creates a larger, but
self-contained, file.

MMD will automatically attempt to *Store* assets in certain file formats
(e.g. EPUB, TextBundle, TextPack).  In these cases, the assets (CSS, images)
are renamed to a UUID and stored inside the generated package.

Finally, MMD can attempt to *Download* (`-D`) assets using [cURL] if it is
available when MMD is compiled on your machine.  With this option, MMD will
attempt to download CSS and image files for embedding.


### HTML ###

Markdown converts text into HTML.  MMD extended this to include other output
formats, but HTML will always be the first.

If no output format is specified, HTML is assumed.

    multimarkdown input.txt > output.html


### MMD ###

Sometimes it is useful to process a MMD document, but output the file back to
plain MMD text rather than a different output format.

    multimarkdown -t mmd input.txt > output.mmd


### AST ###

If you're using MMD as a library in your own software, it may be useful at
times to be able to see how MMD parses a file.  The AST output format shows
the result of the initial parsing.

    multimarkdown -t ast input.txt > output.txt
    multimarkdown ast input.txt > output.txt


### Hash ###

MMD can calculate a hash value for each node in the AST to allow you to
quickly and easily compare an individual node (and its children) for
equivalence to another node.

There is a trade-off here, however.  A hash value is most useful when it can
be quickly calculated and used to compare different objects.  The longer it
takes to calculate the hash value, the smaller the benefit becomes.  A
trade-off that I made here was that the hash value is not based on
the *content* of the node, but rather the structure of the node and its
children.

There are a few key considerations:

* A *block* node has a starting offset compared to the beginning of the
  document. Inserting a paragraph at the top of a document will alter the
  starting offset of every subsequent block node, despite no changes to those
  blocks occuring.

* Children of block nodes have a starting offset compared to the beginning of
  the parent block. Inserting a paragraph at the top of the document
  will *not* affect the starting offsets of child nodes within subsequent
  blocks.

* Calculating a hash based on the actual text content is possible, but
  relatively slow since each character has to be calculated into the hash.

So, for now, the hash is calculated based on:

* The length of the node under consideration
* The starting offset of each child and content node
* The length of each child and content node
* The hash value of each child and content node

I *believe* that this will be sufficient for most needs, but will need further
testing to be sure.  My goal is to see whether these hash values will be
useful for quickly identifying portions of a complex document that have
changed and may need updating, without having to fully re-process the entire
document.  This is less of a concern when converting MMD source text into a
different format (e.g. HTML) since the process is quite fast as is.  But when
performing syntax highlighting, for example, it is much faster to minimize
what has to be updated.

More to come on this....

    multimarkdown -t hash input.txt > output.txt
    multimarkdown hash input.txt > output.txt


### LaTeX ###

[LaTeX] is a system for typesetting documents, using the [TeX] system. It is a
very complex, but highly functional, system that can generate high quality
PDFs, among other things.

MMD simplifies this by allowing you to ignore much of the behind the scenes
details for some common layout options.

You must have LaTeX installed, along with a variety of packages, which is
beyond the scope of this document.

MMD comes with a small number of LaTeX packages that simplify the process.
These should be installed in the usual location for your operating system.

To generate a LaTeX file that works properly, you should include three key
pieces of metadata:

    latex class:    <desired class, often article, memoir, or beamer>
    latex class options:    <any desired options for your chosen class, if any>
    latex package:    mmd7-article, mmd7-letterhead, mmd7-beamer, mmd7-core, or your own custom package

Basically, you use the combination of `latex class`, `latex class options`,
and primary `latex package`, along with your document's other metadata
(such as `title`, `author`, etc.) to determine how a document is processed.

If you need to pass options to the package, you can also use something like:

    latex package: [options]mmd7-core

Note that if you are generating a document for processing with `beamer`, you
need to use the `beamer` output format rather than the regular `latex`
format.

When generating LaTeX documents, you need to pay attention to which header
levels are used in your document.  You may find it easiest to always start
with `<h1>` as your top level, and use the `base header level` to adjust it
as needed to work with your desired LaTeX class.

Again, remember that metadata can be included at the top of the document, or
prepended by a script.  This gives you several approaches to make it possible
to control this process in the manner that works best for you.

    multimarkdown -t latex input.txt > output.tex
    multimarkdown -t beamer input.txt > output.tex


### OPML/iThoughts ###

OPML is a useful format when you desire to work with a MMD document in an
outliner or mind-mapping application.  For example, I sometimes find it
useful to jump back and forth between a regular text editor and an outliner
when I am writing longer documents.

To assist with this, MMD does two things:

1. It can export your text document to an OPML or iThoughts document format

2. It can read *from* an OPML or iThoughts document to convert your file back to
MMD text.  This allows you to "round trip" between applications.

For example:

    multimarkdown -t opml input.txt > output.opml
    multimarkdown -O -t mmd input.opml > output.mmd

    multimarkdown -t itmz input.txt > output.itmz
    multimarkdown -I -t mmd input.itmz > output.mmd

iThoughts was a mind-mapping application for iOS and macOS that was fantastic.
It is no longer being developed, and I will be quite sad on the day when it
ceases to work on my devices...  I still have yet to find another
mind-mapping program that I like as much.


### TextBundle ###

[TextBundle] is a package format for macOS/iOS that effectively stores your
file inside a directory named with a specific extension.  This allows you to
include various assets, such as CSS and image files, inside the directory.

TextPack is similar, but is compressed as a zip file.

MMD can create both formats.

    multimarkdown -t textbundle input.txt > output.textbundle
    multimarkdown -t textpack input.txt > output.textpack


### EPUB ###

MMD can generate EPUB v3 e-book files.  These are basically a zipped package
of files, using an XHTML file as the central document.

    multimarkdown -t epub input.txt > output.epub

(There are plenty of tools out there to interconvert e-book formats, so once
you have an EPUB, you can convert it to other formats as desired.  I do not
intend on supporting other e-book formats natively within MMD.)


## Core Markdown Features ##
### Headers ###

MMD headers are defined like Markdown headers, but have an extra feature.  MMD
generates `id` attributes for each header (based on the title of the header).
This allows you to link to each header in the document.  The TOC takes
advantage of this to allow navigating your document easily, but you can also
create your own links to specific sections of your document.

    # Header One #

    Link to [Header One][].

You can also manually choose an `id` to disambiguate cases where more than one
header has the same title.

    # Header One [ThisOne] #

### HTML Blocks ###

TODO


### HTML Spans ###

TODO


## Extended MultiMarkdown Features ##
### Metadata ###

TODO include common keys.

### Table of Contents ###

You can use `{{TOC}}` as a paragraph by itself to insert a TOC at that location.


### Fenced Code Blocks ###

In addition to indented code blocks, MMD supports fenced code blocks.

    ```
    This is a code block
    ```

The number of backticks before and after the code block must match.


TODO: Add language specifier details


### Definition Lists ###

MultiMarkdown has support for definition lists using the same syntax used in
[PHP Markdown Extra][]. Specifically:

    Apple
    :    Pomaceous fruit of plants of the genus Malus in 
        the family Rosaceae.
    :    An american computer company.
    
    Orange
    :    The fruit of an evergreen tree of the genus Citrus.


becomes:

> Apple
> : Pomaceous fruit of plants of the genus Malus in 
>        the family Rosaceae.
> : An american computer company.
>
> Orange
> : The fruit of an evergreen tree of the genus Citrus.

You can have more than one term per definition by placing each term on a
separate line. Each definition starts with a colon, and you can have more than
one definition per term. You may optionally have a blank line between the last
term and the first definition.

Definitions may contain other block level elements, such as lists,
blockquotes, or other definition lists.

See the [PHP Markdown Extra][] page for more information.


### Tables ###
### Figures ###
### Footnotes ###
### Citations ###
### Glossaries ###
### Abbreviations ###
### Link Attributes ###
### Math ###
### Smart Quotes ###

MultiMarkdown converts "plain" punctuation into "smarter" typographic punctuation:

* Straight quotes (`"` and `'`) into "curly" quotes 
* Backticks-style quotes (` ``this'' `) into "curly" quotes
* Dashes (`--` and `---`) into en- and em- dashes
* Three dots (`...`) become an ellipsis

MultiMarkdown also includes support for quotes styles other than English (the default).  Use the `quotes language` metadata to choose:

* English (`en`)
* Dutch (`nl`)
* German(`de`)
* German guillemets(`germanguillemets`)
* French(`fr`)
* Spanish(`es`)
* Swedish(`sv`)


### File Transclusion ###


## CriticMarkup Support ##
### What is CriticMarkup? ###
### CriticMarkup Syntax ###
### Options for CriticMarkup ###
### CriticMarkup Limitations ###


[>AST]: Abstract Syntax Tree
[>MMD]: MultiMarkdown
[>OPML]: Outliner Processor Markup Language
[>UUID]: Universally Uniquie Identifier
[>TOC]: Table of Contents

[beamer]: https://ctan.org/pkg/beamer "Beamer LaTeX class"
[cmake]: https://cmake.org "CMake"
[cURL]: https://github.com/curl/curl "cURL"
[LaTeX]: https://www.latex-project.org "LaTeX Project"
[Markdown]: http://daringfireball.net/projects/markdown/ "Markdown"
[MultiMarkdown]: https://fletcherpenney.net/multimarkdown/ "MultiMarkdown"
[PHP Markdown Extra]:    http://www.michelf.com/projects/php-markdown/extra/ "PHP Markdown Extra"
[repo]:    https://github.com/fletcher/MultiMarkdown-7 "MultiMarkdown GitHub Repository"
[TeX]: https://tug.org "TeX User's Group"
[TextBundle]: https://textbundle.org "TextBundle"
