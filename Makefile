# Compile the MultiMarkdown User's Guide into documentation or test suite

srcfiles := $(wildcard *.md)

htmlfiles := $(patsubst %.md, %.html, $(srcfiles))

epubfiles := $(patsubst %.md, %.epub, $(srcfiles))

rtffiles := $(patsubst %.md, %.rtf, $(srcfiles))

texfiles := $(patsubst %.md, %.tex, $(srcfiles))

odffiles := $(patsubst %.md, %.fodt, $(srcfiles))

pdffiles := $(patsubst %.md, %.pdf, $(srcfiles))

examples := $(wildcard examples/*.text)

ex-html := $(patsubst %.text, %.html, $(examples))

ex-rtf := $(patsubst %.text, %.rtf, $(examples))

ex-tex := $(patsubst %.text, %.tex, $(examples))

ex-fodt := $(patsubst %.text, %.fodt, $(examples))



all: $(htmlfiles) $(texfiles) epub fodt # $(rtffiles) $(odffiles)

html: $(htmlfiles)


%.html: %.md
	./build/multimarkdown batch -t html -r $*.md

%.html: %.text
	./build/multimarkdown batch -t html -r $*.text

%.epub: %.md
	./build/multimarkdown batch -t epub -r $*.md

%.epub: %.text
	./build/multimarkdown batch -t epub -r $*.text

%.rtf: %.md
	./build/multimarkdown batch -t rtf -r $*.md

%.rtf: %.text
	./build/multimarkdown batch -t rtf -r $*.text

%.tex: %.md
	./build/multimarkdown batch -t latex -r $*.md

%.tex: %.text
	./build/multimarkdown batch -t latex -r $*.text

%.fodt: %.md
	./build/multimarkdown batch -t fodt -r $*.md

%.fodt: %.text
	./build/multimarkdown batch -t fodt -r $*.text


examples: $(ex-html) $(ex-rtf) $(ex-tex) $(ex-odf)


clean:
	@rm $(htmlfiles) $(rtffiles) $(texfiles) $(odffiles) $(pdffiles) $(epubfiles)

clean-examples:
	@rm $(ex-html) $(ex-rtf) $(ex-tex) $(ex-odf)
