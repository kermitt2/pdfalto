# pdfalto

pdfalto parses PDF files and produces an XML representations in ALTO format. 

pdfalto is a fork of pdf2xml http://sourceforge.net/projects/pdf2xml, developed at XRCE, with modifications for robustness, addition of features and output format in ALTO.  

The latest stable release is version *0.1*. 

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


## (stable)

This following modifications have been made:

- encode URI (using `xmlURIEscape` from libxml2) for the @href attribute content to avoid blocking XML wellformedness issues. From our experiments, this problem happens in average for 2-3 scholar PDF out of one thousand.

- output coordinates attributes for the BLOCK elements when the `-block` option is selected,

- add a parameter `-readingOrder` which re-order the blocks following the reading order when the -block option is selected. By default in pdf2xml, the elements follow the PDF content stream (the so-called _raw order_). In pdf2txt from xpdf, several text flow orders are available including the raw order and the reading order. Note that, with this modification and this new option, only the blocks are re-ordered.

From our experiments, the raw order can diverge quite significantly from the order of elements according to the visual/reading layout in 2-4% of scholar PDF (e.g. title element is introduced at the end of the page element, while visually present at the top of the page), and minor changes can be present in up to 100% of PDF for some scientific publishers (e.g. headnote introduced at the end of the page content). This additional mode can be thus quite useful for information/structure extraction applications exploiting pdf2xml output. 

- use the latest version of xpdf, version 3.04.

### Compilation

See the file INSTALL for more details. 

    
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

The pdf2xml version has been modified by Patrice Lopez (patrice.lopez@science-miner.com).

The first windows version (1.0.0) has been built by [@pboumenot](https://github.com/boumenot) and ported on windows 7 for 64 bit. 

The version 2.0 for windows (native and cygwin) was built by [@lfoppiano](https://github.com/boumenot) and [@flydutch](https://github.com/flydutch).  

## License

As the original pdf2xml, GPL2. 







