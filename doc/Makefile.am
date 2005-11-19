
dist_noinst_DATA=FAQ.html
#noinst_DATA=FAQ.pdf FAQ.ps

EXTRA_DIST=FAQ.xml
CLEANFILES=*.aux *.dvi *.fo *.log *.out
MAINTAINERCLEANFILES=FAQ.html FAQ.pdf FAQ.ps
STYLESHEET_DIR=/usr/share/sgml/docbook/xsl-stylesheets
HTML_STYLESHEET=$(STYLESHEET_DIR)/html/docbook.xsl
FO_STYLESHEET=$(STYLESHEET_DIR)/fo/docbook.xsl

SUFFIXES=.xml .fo .dvi .html .ps .pdf
.PHONY: validate

.fo.pdf:
	pdfxmltex $<

.fo.dvi:
	xmltex $<

.dvi.ps:
	dvips -o $@ $<

.xml.html:
	xsltproc -o $@ $(HTML_STYLESHEET) $<

.xml.fo:
	xsltproc -o $@ $(FO_STYLESHEET) $<

validate:
	xmllint --valid --noout *.xml
