### Text serialization from ALTO XML file(s)

* Simple XSLT 1.0 version, not supporting RTL languages:

For instance, using `xsltproc` command line, the following outputs the text content only:

> xsltproc schema/alto2txt.xsl alto_file.xml

* Experimental XSLT 2.0 version, supporting both LTR and RTL languages:

Require an XSLT 2.0 processor, e.g. `saxon`:

> java -jar saxon9he.jar -s:alto_file.xml -xsl:schema/alto2txt-rl.xsl -o:out.txt -dtd:off -a:off -expand:off --parserFeature?uri=http%3A//apache.org/xml/features/nonvalidating/load-external-dtd:false -t

