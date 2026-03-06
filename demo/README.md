# MultiMarkdown Demo Documents #

This folder contains some sample documents for demonstrating and testing
several features.

These files contain basically the same minimal contact, but have different
ranges of headers.  Some make use of the `base header level` metadata to
control how things work :

* src/deep.mmd		-- Contains h1 - h4
* src/medium.mmd	-- Contains h1 - h3
* src/shallow.mmd	-- Contains h1 - h2

This file integrates most MultiMarkdown features in a shallow document:

* src/integration.mmd -- Contains h1 - h2

Another is an old introduction to MultiMarkdown.  The content is dated, but it
demonstrates various functions:

* src/flat.mmd		-- Contains h1 only

Lastly, `letter.mmd` has no headers, just lorem ipsum text.

Finally, some scripts also use the Integrated test suite file to include a
fairly extensive collection of MultiMarkdown features for testing various
output formats.


There are multiple scripts:

* article.sh		-- Build a general article document using 3 different approaches in LaTeX
* beamer.sh			-- Build slideshow using LaTeX and Beamer
* book.sh			-- Build a book using LaTeX and standard book class
* epub.sh			-- Build EPUB 3 ebook
* html.sh			-- Build HTML files
* itmz.sh			-- Build iThoughts Mind-Map file
* letterhead.sh		-- Build letter using LaTeX and letter class
* opml.sh			-- Build OPML outline
* report.sh			-- Build a report using LaTeX and standard report class
* sffms.sh			-- Build manuscript using LaTeX and sffms class
* tufte-book.sh		-- Build book using LaTeX and tufte-book class
* tufte-handout.sh		-- Build handout using LaTeX and tufte-handout class
