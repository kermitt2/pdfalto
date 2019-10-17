Here we describe procedures to compile the static dependencies.

# Requirements

* compilers : clang 3.6 or gcc 4.9
* makefile generator : cmake 3.12.0
* fetching dependencies : wget

# libpng & zlib
1- The source code for these libraries could be found under `libs/image/png/src` and `libs/image/zlib/src`, so we start with zlib :
> cd libs/image/zlib/src && cmake ./ && make && cd -

2- then png library :
> cd libs/image/png/src && cmake ./ && make && cd -

3- This will generate .a static libraries to be copied under corresponding OS `libs/image/png/<OS>`
# libxml2

1- Download the source from http://xmlsoft.org/sources/libxml2-2.9.8.tar.gz and unpack it.
2- Access the directory then autoreconf for generating the configure file :
> autoreconf

(you might need to use dos2unix to fix encoding problem from windows)

3- Once `configure` is produced :

> ./configure --disable-dependency-tracking --without-python --without-lzma --without-iconv --without-zlib

4- Last run `make` to compile, then take the static file from `.libs/libxml2.a` and paste it under corresponding OS `libs/libxml/<OS>`

# freetype

1- Download the source from https://sourceforge.net/projects/freetype/files/ and unpack it (last tested version 2.9.1).
2- Access the directory then create a build directory :
> mkdir _build && cd _build

(you might need to use dos2unix to fix encoding problem from windows)

> cmake -G "Unix Makefiles" ../ -DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=TRUE -DFT_WITH_ZLIB=OFF -DFT_WITH_BZIP2=OFF -DFT_WITH_PNG=OFF

(options are set to avoid searching libraries)

3- Finally run `make` , then copy the static file from `libfreetype.a` and paste it under corresponding OS  `libs/freetype/<OS>`

# ICU
After downloading sources (tested with version 62) from http://site.icu-project.org/download/ :

1- Decompress the archive file. For example,
> gunzip -d < icu-X.Y.tgz | tar xvf -

2- Change directory to icu/source.
>cd icu/source && mkdir lib && mkdir bin

3- Some files may have the wrong permissions.
>chmod +x runConfigureICU configure install-sh

(using cygwin from windows, you might need to use dos2unix to fix encoding problem or correct manually)

4- Run the runConfigureICU script for your platform (static library should be generated).
>./runConfigureICU <MacOSX | Linux | Cygwin>  --enable-static --disable-shared

5- Now build:
>make

See [ICU Readme](http://source.icu-project.org/repos/icu/trunk/icu4c/readme.html) for futher details.

6- Copy `libicuuc.a` from `icu/source/lib` and `libicudata.a` from `icu/source/stubdata` and put it under corresponding OS `libs/icu/<OS>`
# Contributors

Main contact: Patrice Lopez (patrice.lopez@science-miner.com)

pdfalto is developed by Patrice Lopez (patrice.lopez@science-miner.com) and Achraf Azhar (achraf.azhar@inria.fr).

xpdf is developed by Glyph & Cog, LLC (1996-2017) and distributed under GPL2 or GPL3 license. 

pdf2xml is orignally written by Hervé Déjean, Sophie Andrieu, Jean-Yves Vion-Dury and  Emmanuel Giguet (XRCE) under GPL2 license. 

The windows version has been built originally by [@pboumenot](https://github.com/boumenot) and ported on windows 7 for 64 bit, then for windows (native and cygwin) by [@lfoppiano](https://github.com/lfoppiano) and [@flydutch](https://github.com/flydutch).  

# License

As the original pdf2xml, pdfalto is distributed under GPL2 license. 
