# pdfalto

pdfalto parses PDF files and produces an XML representations in ALTO format. 

pdfalto is a fork of pdf2xml http://sourceforge.net/projects/pdf2xml, developed at XRCE, with modifications for robustness, addition of features and output format in ALTO.  



## pdf2xml

This is a modified version of pdf2xml available at http://sourceforge.net/projects/pdf2xml developed by XRCE (Hervé Déjean, Sophie Andrieu, Jean-Yves Vion-Dury, Emmanuel Giguet).

The latest stable release is version *0.1*. 

## version 2.1 (development)
- Adding CMake Build

- more to come!

### Compilation

See the file INSTALL for more details. 

### Linux

* Install libxml2 (development headers). See http://xmlsoft.org/ 

* Install libmotif-dev (development headers) 

* Xpdf is shipped as git submodule, to download it: 

> git submodule update --init --recursive

* To build: 

> cmake .

> make
    
### Windows 

See `Compilation` for version 2.0 (at the moment nothing has changed)


## version 2.0 (stable)
This following modifications have been made:

- encode URI (using `xmlURIEscape` from libxml2) for the @href attribute content to avoid blocking XML wellformedness issues. From our experiments, this problem happens in average for 2-3 scholar PDF out of one thousand.

- output coordinates attributes for the BLOCK elements when the `-block` option is selected,

- add a parameter `-readingOrder` which re-order the blocks following the reading order when the -block option is selected. By default in pdf2xml, the elements follow the PDF content stream (the so-called _raw order_). In pdf2txt from xpdf, several text flow orders are available including the raw order and the reading order. Note that, with this modification and this new option, only the blocks are re-ordered.

From our experiments, the raw order can diverge quite significantly from the order of elements according to the visual/reading layout in 2-4% of scholar PDF (e.g. title element is introduced at the end of the page element, while visually present at the top of the page), and minor changes can be present in up to 100% of PDF for some scientific publishers (e.g. headnote introduced at the end of the page content). This additional mode can be thus quite useful for information/structure extraction applications exploiting pdf2xml output. 

- use the latest version of xpdf, version 3.04.

### Compilation

See the file INSTALL for more details. 

### Linux

* Install libxml2 (development headers). See http://xmlsoft.org/ 

* Install libmotif-dev (development headers) 

* Download xpdf. See http://www.foolabs.com/xpdf/

* The makefile has been adapted to work with the following directory format
(iconv, libxml2 and dirent root dirs should be at the same level of pdf2xml's sources):  

```bash
drwxr-xr-x 1 lfoppiano 197121 0 ago  1 10:44 pdf2xml/ (<- pdf2xml source)
drwxr-xr-x 1 lfoppiano 197121 0 lug 28 09:06 xpdf-3.04/
```

* Compile xpdf (see the file INSTALL in their source directory), then create `libxpdf.a` in `xpdf-XX/xpdf/` with 

> ar -rc libxpdf.a *.o

* Compile the zlib and png libraries, under subdirectory `/images`: 

> make -f <MAKEFILE>
(MAKEFILE being make.bat or make.linux64)

* Update Makefile.linux (paths to xpdf) 

For Linux 32 bits:

> make -f Makefile.linux

Use Makefile.linux64 for last version on Linux 64bits

> make -f Makefile.linux64

This should build the executable:

    exe/pdftoxml.exe
    
### Windows 

*NOTE: this version seems to have some problems with certain pdf, we 
recommend you to use the version built using cygwin (same process as Linux).

If you feel like discovering the issue, we would much appreciate it ;-)*
 
This guide compile pdf2xml using the native libraries of Windows:  

* Install the Visual Studio Community edition and the tools to build C/C++ applications under windows. 
To verify make sure the command `cl.exe` and `lib.exe` are found.   

* Download iconv from  https://sourceforge.net/projects/gettext/files/libiconv-win32/1.9.1/

* Download libxml2 from﻿ ftp://xmlsoft.org/libxml2/win32/

* Download the library dirent from﻿ https://github.com/tronkko/dirent

* Download xpdf from  http://www.foolabs.com/xpdf/

* The makefile has been adapted to work with the following directory format
(iconv, libxml2 and dirent root dirs should be at the same level of pdf2xml's sources):  

```bash
drwxr-xr-x 1 lfoppiano 197121 0 lug 28 17:41 dirent/
drwxr-xr-x 1 lfoppiano 197121 0 ago  1 10:38 libiconv-1.9.1/
drwxr-xr-x 1 lfoppiano 197121 0 lug 30 20:02 libxml2-2.7.8.win32/
drwxr-xr-x 1 lfoppiano 197121 0 ago  1 10:44 pdf2xml/ (<- pdf2xml source)
drwxr-xr-x 1 lfoppiano 197121 0 lug 28 09:06 xpdf-3.04/
``` 

* Build xpdf using the windows ms_make.bat.  

* create `libxpdf.a` in `xpdf-XX/xpdf/` with 

> lib /out:libxpdf.lib *.obj

* Compile the zlib and png libraries, under the /images subdirectory in pdf2xml source: 

> make.bat

### MacOS

* Download the source with [Xpdf](https://github.com/kermitt2/xpdf) shipped as submodule using git:

> git clone --recursive https://github.com/Aazhar/pdfalto.git

* In order to compile Xpdf, we’ll need to have libxml, freestyle, X11 (xquartz on mac) openmotif installed (using home-brew for instance) and added to the environment (accessible from terminal)

* Compile xpdf (see the file INSTALL in their source directory), then create `libxpdf.a` in `xpdf/xpdf/` with 

> ar -rc libxpdf.a *.o

* Compile the first zlib then png libraries (dependent), under subdirectory `/images`:

> cmake CMakeList.txt

> make -f Makefile 

* Execute the compilation to generate pdfalto using :

> make -f Makefile.macos 

## Contributor

This pdf2xml version has been modified by Patrice Lopez (patrice.lopez@science-miner.com).

The first windows version (1.0.0) has been built by [@pboumenot](https://github.com/boumenot) and ported on windows 7 for 64 bit. 

The version 2.0 for windows (native and cygwin) was built by [@lfoppiano](https://github.com/boumenot) and [@flydutch](https://github.com/flydutch).  

## License

As the original pdf2xml, GPL2. 


## Original readme 
====

pdftoxml
====

version 1.0.0
July 2007 

The Xpdf software and documentation are
copyright 1996-2007 Glyph & Cog, LLC.

Email: derekn@foolabs.com
WWW: http://www.foolabs.com/xpdf/

The PDF data structures, operators, and specification are
copyright 1985-2006 Adobe Systems Inc.

The libxml2 software and documentation are released under the MIT License. 
See the Copyright file in the distribution for the precise wording.

What is pdftoxml?
-------------

pdftoxml is an open source PDF to XML convertor.  
pdftoxml runs under Linux and on Win32 systems.

pdftoxml is based on xpdf and is essentially a (large) modification 
of pdftotext in order to generate XML instead of plain text. 
The XML generation uses the libxml2 library


Distribution
------------

pdftoxml is licensed under the GNU General Public License (GPL), version 2.  


Compatibility
-------------

pdftoxml is developed and tested on a Linux 2.4 x86 system.
In addition, it has been compiled on a Win32 system. 

Getting pdftoxml
------------

The latest version is available from: https://sourceforge.net/projects/pdf2xml
Source code is available from: http://pdf2xml.cvs.sourceforge.net/pdf2xml/


Running pdftoxml
------------

To run pdftoxml, simply type:

  pdftoxml.exe file.pdf

Command line options and many other details are(should be) described in sourceforge


Compiling pdftoxml
--------------

See the separate file, INSTALL.


Contributors
----

Hervé Déjean   (src) 
Sophie Andrieu (src)
Jean-Yves Vion-Dury (schemas)






