
dist_noinst_DATA=FAQ.html
#noinst_DATA=FAQ.pdf FAQ.ps

EXTRA_DIST=FAQ.xml
CLEANFILES=*.aux *.dvi *.fo *.log *.out
MAINTAINERCLEANFILES=FAQ.html FAQ.pdf FAQ.ps

SUFFIXES=.xml .fo .dvi .html .ps .pdf
.PHONY: validate

.fo.pdf:
	pdfxmltex $<

.fo.dvi:
	xmltex $<

.dvi.ps:
	dvips -o $@ $<

.xml.html:
	xsltproc --docbook -o $@ /usr/share/sgml/docbook/xsl-stylesheets/html/docbook.xsl $<

.xml.fo:
	xsltproc --docbook -o $@ /usr/share/sgml/docbook/xsl-stylesheets/fo/docbook.xsl $<

validate:
	xmllint --valid --noout *.xml
