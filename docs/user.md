title:	MultiMarkdown User's Guide
author:	Fletcher T. Penney
version:	7.0.0
revised:	2026-03-13
baseheaderlevel:	1
css:	css/Classless.min.css
htmlheader: <style>main {display: grid; grid-template-columns: 16em 45em; margin: 0 auto; gap: 20px;}
    main > nav {position: sticky; align-self: start; top: 2rem; animation-timeline: view();}
    del { background: #fae6e6; } ins { background: #ecfce6; } span.critic.comment { color: #0000bb; }
    span.critic.comment::before { content: "{>> "; } span.critic.comment::after { content: " \3C\3C}"; }
    </style>
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


[#Gruber]: John Gruber.  Daring Fireball: Markdown. [Cited January 2006].
Available from <http://daringfireball.net/projects/markdown/>.

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

Installers for macOS and archived source files for each release are available
on the Github repository [Releases] page.

The source code for MMD is available on [Github][repo].

(*NOTE*: At this time I don't have a convenient way to package installers for
Windows for various machine architectures.  I am open to suggestions.  It
looks like it *might* be possible to do this on Github itself, but I'll need
to dig into this further.)


### How to Compile MultiMarkdown ###

MMD uses [Cmake][cmake] as the underlying build system.  Compiling
proceeds in two stages:

    make
    cd build
    make

You can test to ensure that everything works correctly:

    ctest


### How to Install MultiMarkdown ###

You can install your own build:

    make
    cd build
    make
    make install


Or you can use a pre-built installer from the Github
[releases page](https://github.com/fletcher/MultiMarkdown-7/releases).

(Currently the installers are for macOS only.  I don't have a workflow yet to
generate Windows installers.)


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


### Command-Line Changes ###

MMD v7 handles arguments from the command-line in a slightly different way
from v6, though the most common use cases are unchanged.

  multimarkdown [--help] {ast|batch|hash|meta|parse} [options] [Input file names]

The first argument should be an action.  If no action is specified, MMD
defaults to the `parse` action.

* `ast` -- Display the AST showing how the document was parsed

* `batch` -- Parse each file individually and write to the same filename with
  a new extension

* `hash` -- Display the hash tree for the parsed document

* `meta` -- Extra metadata from the document without parsing the rest

* `parse` -- Parse the document(s) and export to the desired format


You can then specify different options to adjust the default behavior:

* `-t FORMAT` -- Specify the output format (`html`, `mmd`, `latex`, `docx`,
  `epub`, `itmz`, `opml`, `textbundle`, `textpack`, `ast`, `hash`)

* `-o OUT_FILE` -- Specify the filename for writing the output

* `-l LANGUAGE` -- Specify the language for smart quotes and default markup
  (`en`, `es`, `de`, `fr`, `nl`, `sv`, `he`)

* `-c` -- Markdown compatibility mode -- turn off features that are not in the
  original Markdown specification.  Note that due to bugs in the original
  `Markdown.pl` and due to ambiguities in the original "spec", the output
  will not exactly match John Gruber's `Markdown.pl`.

* `-C` -- Generate a complete document

* `-S` -- Generate a snippet

* `-r` -- Enable file transclusion ("recursive")

* `-D` -- Download assets from the internet (images, CSS) for inclusion in
  package formats (requires libcurl when compiling)

* `-E` -- Embedd assets in non-package formats where possible (e.g. embed
  images directly in HTML as Base64)

* `-p PATH` -- Specify a working directory when parsing from stdin (e.g. for
  transclusion or embedding assets)

* `-A` -- Accept all CriticMarkup changes

* `-R` -- Reject all CriticMarkup changes

* `-O` -- Convert OPML source to MMD text before parsing

* `-I` -- Convert iThoughts source to MMD text before parsing

* `-e` -- Specify metadata key to extract value from MMD source

* `-b` -- Limit parsing to block level only (useful for diagnostics only)

* `-s` -- Log some processing time statistics (does not work on Windows)


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

* Compatibility mode (`-c`) -- disables most functionality that was not in the
  original `Markdown.pl` script.

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
  in your own application. (NOTE: This is really only useful with the AST or
  Hash output formats, since each block will be empty.)


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
formats as well.

If no output format is specified, HTML is assumed.

    multimarkdown input.txt > output.html
    multimarkdown parse input.txt > output.html


### MMD ###

Sometimes it is useful to process a MMD document, but output the file back to
plain MMD text rather than a different output format.

    multimarkdown -t mmd input.txt > output.mmd
    multimarkdown parse -t mmd input.txt > output.mmd


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

Just like regular Markdown, MMD supports including raw HTML blocks within your
document.  The key difference is that MultiMarkdown *will* parse inside HTML
blocks that include a blank line between the opening HTML tag and the content.

MMD will *not* parse inside this:

    <del>
    *foo*
    </del>

but will parse inside this:

    <del>

    *foo*
    </del>

(This difference in behavior allows you to control what is treated as Markdown
and what is treated as raw HTML by adjusting the spacing as necessary.)


### HTML Spans ###

HTML spans are raw HTML that is within a single MultiMarkdown block, such as a
paragraph.  MultiMarkdown *will* parse inside these spans.

    foo
    <del>
    *bar*
    </del>

    foo <del>*bar*</del>


## Extended MultiMarkdown Features ##
### Metadata ###

TODO include common keys.

You can use MMD to extract metadata from source text:

    multimarkdown meta input.txt
    multimarkdown meta -e key_name input.txt


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

TODO


### Figures ###

TODO


### Footnotes ###

TODO


### Citations ###

TODO


### Glossaries ###

TODO


### Abbreviations ###

TODO


### Link Attributes ###

TODO


### Math ###

TODO


### Smart Quotes ###

MultiMarkdown converts "plain" punctuation into "smarter" typographic punctuation:

* Straight quotes (`"` and `'`) into "curly" quotes 
* Backticks-style quotes (` ``this'' `) into "curly" quotes
* Dashes (`--` and `---`) into en- and em- dashes
* Three dots (`...`) become an ellipsis

MultiMarkdown also includes support for quotes styles other than English
(the default).  Use the `quotes language` metadata to choose:

* English (`en`)
* Dutch (`nl`)
* German(`de`)
* German guillemets(`germanguillemets`)
* French(`fr`)
* Spanish(`es`)
* Swedish(`sv`)


### File Transclusion ###

TODO


## CriticMarkup Support ##
### What is CriticMarkup? ###

> CriticMarkup is a way for authors and editors to track changes to documents
> in plain text. As with Markdown, small groups of distinctive characters
> allow you to highlight insertions, deletions, substitutions and comments,
> all without the overhead of heavy, proprietary office suites.
> <http://criticmarkup.com/> 

CriticMarkup is integrated with MultiMarkdown itself, as well as
[MultiMarkdown Composer].  I encourage you to check out the [CriticMarkup] web
site to learn more as it can be a very useful tool.  There is also a great
video showing CriticMarkup in use while editing a document in MultiMarkdown
Composer. 


### Using CriticMarkup with MultiMarkdown ###

When using CriticMarkup with MultiMarkdown itself, you have three choices: 

*   Leave the CriticMarkup syntax in place (`multimarkdown foo.txt`).
    MultiMarkdown will attempt to show the changes as highlights in the
    exported document, where possible.  This will not *always* result in a
    valid output document.

*   **Accept** all changes, giving you the "new" document
    (`multimarkdown -A foo.txt`)

*   **Reject** all changes, giving you the "original" document
    (`multimarkdown -R foo.txt`)

*   CriticMarkup comments and highlighting are ignored when processing with
    `-A` or `-R`. 


### CriticMarkup Syntax ###

The CriticMarkup syntax is fairly straightforward.  The key thing to remember
is that CriticMarkup is processed *before* any other MultiMarkdown is
handled.  It's almost like a separate layer on top of the MultiMarkdown
syntax. 

When editing in MultiMarkdown Composer, you can have CriticMarkup syntax
flagged in the both the editor pane and the preview window.  This will allow
you to see changes in the HTML preview. 

*   Deletions from the original text: 

        This is {--is --}a test.

    This is {--is --}a test.

*   Additions: 

        This {++is ++}a test.

    This {++is ++}a test.

*   Substitutions: 

        This {~~isn't~>is~~} a test.

    This {~~isn't~>is~~} a test.

* Highlighting: 

        This is a {==test==}.

    This is a {==test==}.

*   Comments: 

        This is a test{>>What is it a test of?<<}.

    This is a test{>>What is it a test of?<<}.


### CriticMarkup limitations ###

If you accept or reject CriticMarkup changes, then it should work properly in
any document.

If you want to try to include your changes as "markup notes" in the final
document, then certain situations will lead to results that were probably not
what you intended.

1.  CriticMarkup must be contained within a single block (e.g. paragraph, list
item, etc.)  CM that spans multiple blocks will not be recognized.

2.  CriticMarkup that crosses multiple MMD spans (e.g. `{++** foo} bar**`)
will not properly manage the intended MultiMarkdown markup.  This example
would not result in bold being applied to `foo bar`.


### My philosophy on CriticMarkup ###

I view CriticMarkup as two things (in addition to the actual tools that
implement these concepts): 

1.  A syntax for documenting editing notes and changes, and for collaborating
amongst coauthors. 

2.  A means to display those notes/changes in the HTML output. 

I believe that #1 is a really great idea, and well implemented.  #2 is not so
well implemented, largely due to the "orthogonal" nature of CriticMarkup and
the underlying Markdown syntax. 

CM is designed as a separate layer on top of Markdown/MultiMarkdown.  This
means that a Markdown span could, for example, start in the middle of a
CriticMarkup structure, but end outside of it.  This means that an algorithm
to properly convert a CM/Markdown document to HTML would be quite complex,
with a huge number of edge cases to consider.  I've tried a few
(fairly creative, in my opinion) approaches, but they didn't work.  Perhaps
someone else will come up with a better solution, or will be so interested
that they put the work in to create the complex algorithm.  I have no current
plans to do so. 

Additionally, there is a philosophical distinction between documenting editing
notes, and using those notes to produce a "finished" document (e.g. HTML or
PDF) that keeps those editing notes intact (e.g. strikethroughs,
highlighting, etc.) I believe that CM is incredibly useful for the editing
process, but am less convinced for the output process (I know many others
disagree with me, and that's ok.  And to be clear, I think that what Gabe and
Erik have done with CriticMarkup is fantastic!) 

There are other CriticMarkup tools besides MultiMarkdown and
[MultiMarkdown Composer], and you are more than
welcome to use them. 

For now, the *official* MultiMarkdown support for CriticMarkup consists of: 

1.  CriticMarkup syntax is "understood" by the MultiMarkdown parser, and by
MultiMarkdown Composer syntax highlighting.

2.  When converting from MultiMarkdown text to an output format, you can
ignore CM formatting with compatibility mode (probably not what you want to
do), accept all changes, or reject all changes (as above).  These are the
preferred choices.

3.  The secondary choice, is to *attempt* to show the changes in the exported
document.  Because the syntaxes are orthogonal, this will not always work
properly, and will not always give valid output files.


[>AST]: Abstract Syntax Tree
[>MMD]: MultiMarkdown
[>OPML]: Outliner Processor Markup Language
[>PEG]: Parsing Expression Grammar
[>UUID]: Universally Uniquie Identifier
[>TOC]: Table of Contents
[>XSLT]: Extensible Stylesheet Language Transformations

[beamer]: https://ctan.org/pkg/beamer "Beamer LaTeX class"
[cmake]: https://cmake.org "CMake"
[CriticMarkup]: https://github.com/CriticMarkup/CriticMarkup-toolkit "CriticMarkup"
[cURL]: https://github.com/curl/curl "cURL"
[LaTeX]: https://www.latex-project.org "LaTeX Project"
[Markdown]: http://daringfireball.net/projects/markdown/ "Markdown"
[MultiMarkdown]: https://fletcherpenney.net/multimarkdown/ "MultiMarkdown"
[MultiMarkdown Composer]: https://multimarkdown.com/ "MultiMarkdown Composer"
[PHP Markdown Extra]: http://www.michelf.com/projects/php-markdown/extra/ "PHP Markdown Extra"
[Releases]: https://github.com/fletcher/MultiMarkdown-7/releases "MultiMarkdown Releases"
[repo]: https://github.com/fletcher/MultiMarkdown-7 "MultiMarkdown GitHub Repository"
[TeX]: https://tug.org "TeX User's Group"
[TextBundle]: https://textbundle.org "TextBundle"
