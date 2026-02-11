# External Libraries

Here we describe procedures to compile the static dependencies.

## Requirements

* compilers : clang 3.6 or gcc 4.9
* makefile generator : cmake 3.12.0
* fetching dependencies : wget

## Current versions:

- LibPNG: 1.6.47
- zLib: 1.3.1
- libxml2: 2.13.6
- freetype: 2.13.3
- ICU: 76-1

## LibPNG & zLib

LibPNG and zLib sources are downloaded and built as part of the dependency build process.

1. Download and build zlib (tested with 1.3.1):

   ```shell
   wget -O source.tar.gz https://zlib.net/zlib-1.3.1.tar.gz
   tar -zxf source.tar.gz
   cd zlib-*
   mkdir -p build/output
   cd build
   cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/output -DBUILD_SHARED_LIBS=OFF
   make
   make install
   ```

2. Download and build libpng (tested with 1.6.47):

   ```shell
   wget -O source.tar.gz https://sourceforge.net/projects/libpng/files/libpng16/1.6.47/libpng-1.6.47.tar.gz/download
   tar -zxf source.tar.gz
   cd libpng*
   mkdir -p build/output
   cd build
   cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/output -DBUILD_SHARED_LIBS=OFF
   make
   make install
   ```

3. Copy the produced static libraries and headers directories in:

   - `libs/image/zlib/<OS>/<ARCH>/` and `libs/image/zlib/include/`
   - `libs/image/png/<OS>/<ARCH>/` and `libs/image/png/include/`

## libxml2

1. Download the source from https://gitlab.gnome.org/GNOME/libxml2/-/archive/v2.13.6/libxml2-v2.13.6.tar.gz and unpack it.

2. Access the directory and build:

   ```shell
   mkdir -p build/output
   cd build
   cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/output -G "Unix Makefiles" .. -D LIBXML2_WITH_LZMA=OFF -D LIBXML2_WITH_ICONV=OFF -D LIBXML2_WITH_ZLIB=OFF -DBUILD_SHARED_LIBS=OFF
   make
   make install
   ```

3. Copy `build/output/lib/libxml2.a` under `libs/libxml/<OS>/<ARCH>/` and the headers from `build/output/include/libxml2/` under `libs/libxml/include/`.

## Freetype

1. Download the source from https://sourceforge.net/projects/freetype/files/freetype2/2.13.3/ and unpack it.
2. Access the directory then build:
   ```shell
   mkdir -p build/output
   cd build
   ```

   (you might need to use dos2unix to fix encoding problem from windows)

   ```shell
   cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/output -G "Unix Makefiles" -D FT_DISABLE_BROTLI=TRUE -D FT_DISABLE_HARFBUZZ=TRUE -D FT_DISABLE_ZLIB=TRUE -D FT_DISABLE_BZIP2=TRUE -D FT_DISABLE_PNG=TRUE
   make
   make install
   ```

   (options are set to avoid searching libraries)

3. Copy `build/output/lib/libfreetype.a` under `libs/freetype/<OS>/<ARCH>/` and the headers from `build/output/include/freetype2/` under `libs/freetype/include/`.

## ICU

1. Download the sources (tested with version 76-1)
   from https://github.com/unicode-org/icu/archive/refs/tags/release-76-1.tar.gz

2. Decompress the archive file. For example,
   ```shell
   gunzip -d < icu-X.Y.tgz | tar xvf -
    ```

3. Change directory to `icu-release-*/icu4c/source`.
   ```shell
   cd icu-release-*/icu4c/source
   mkdir output
    ```

4. Some files may have the wrong permissions.
    ```shell
   chmod +x runConfigureICU configure install-sh
    ```

(using cygwin from windows, you might need to use dos2unix to fix encoding problem or correct manually)

5. Run the `runConfigureICU` script for your platform and install under the local output prefix.
   ```shell
   ./runConfigureICU <MacOSX | Linux | Cygwin> --enable-static --disable-shared --prefix=$(pwd)/output
   ```

6. Now build and install:
   ```shell
   make
   make install
    ```

See [ICU Readme](http://source.icu-project.org/repos/icu/trunk/icu4c/readme.html) for futher details.

7. Copy `output/lib/libicuuc.a` and `output/lib/libicudata.a` under `libs/icu/<OS>/<ARCH>/` and headers from `output/include/` under `libs/icu/include/`.

# Contributors

Main contact: Patrice Lopez (patrice.lopez@science-miner.com)

pdfalto is developed by Patrice Lopez (patrice.lopez@science-miner.com) and Achraf Azhar (achraf.azhar@inria.fr).

xpdf is developed by Glyph & Cog, LLC (1996-2017) and distributed under GPL2 or GPL3 license.

pdf2xml is originally written by Hervé Déjean, Sophie Andrieu, Jean-Yves Vion-Dury and Emmanuel Giguet (XRCE) under GPL2
license.

The windows version has been built originally by [@pboumenot](https://github.com/boumenot) and ported on windows 7 for
64 bit, then for windows (native and cygwin) by [@lfoppiano](https://github.com/lfoppiano)
and [@flydutch](https://github.com/flydutch).

# License

As the original pdf2xml, pdfalto is distributed under GPL2 license. 
