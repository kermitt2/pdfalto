#!/bin/bash
# Compilation flags
: ${CFLAGS="-fPIC"}
: ${CXX_FLAGS=${CFLAGS}}

set -e

if ! [ -x "$(command -v wget)" ]; then
  echo 'Error: wget is not installed.' >&2
  exit 1
fi

if ! [ -x "$(command -v cmake)" ]; then
  echo 'Error: cmake is not installed.' >&2
  exit 1
fi

DEP_INSTALL_DIR=install

LIBXML_URI=http://xmlsoft.org/sources/libxml2-2.9.8.tar.gz
FREETYPE_URI=https://download.savannah.gnu.org/releases/freetype/freetype-2.9.tar.gz
#ICU_URI=http://download.icu-project.org/files/icu4c/62.1/icu4c-62_1-src.tgz
ICU_URI="https://github.com/unicode-org/icu/releases/download/release-62-2/icu4c-62_2-src.tgz"
#ICU_URI=https://github.com/unicode-org/icu/releases/download/release-66-1/icu4c-66_1-src.tgz

ICU_FILENAME=$(basename "${ICU_URI}")
ICU_DIR_NAME=icu

mkdir -p $DEP_INSTALL_DIR

cd $DEP_INSTALL_DIR

if [[ "$OSTYPE" == "linux-gnu" ]]; then
if ! [ -x "$(command -v autoreconf)" ]; then
        sudo apt-get install autoconf
        fi
        ICU_CONFIG="Linux"
        LIB_INSTALL="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
        # Mac OSX
        if ! [ -x "$(command -v autoreconf)" ]; then
            brew install autoreconf
        fi
        ICU_CONFIG="MacOSX"
        LIB_INSTALL="mac"
elif [[ "$OSTYPE" == "cygwin" ]]; then
        # POSIX compatibility layer and Linux environment emulation for Windows
        ICU_CONFIG="Cygwin"
        LIB_INSTALL="windows"
#elif [[ "$OSTYPE" == "msys" ]]; then
        # Lightweight shell and GNU utilities compiled for Windows (part of MinGW)
#elif [[ "$OSTYPE" == "win32" ]]; then
        # I'm not sure this can happen.
#elif [[ "$OSTYPE" == "freebsd"* ]]; then
        # ...
#else
        # Unknown.
fi

echo "$LIB_INSTALL"

echo 'Installing libxml2.'
#
rm -rf libxml2-2.9.8
wget $LIBXML_URI

tar xvf libxml2-2.9.8.tar.gz

cd libxml2-2.9.8

autoreconf

#Once configure is produced :

./configure --disable-dependency-tracking --without-python --without-lzma --without-iconv --without-zlib

make

cd ..

echo 'libxml2 installation is finished.'


echo 'Installing freetype.'

rm -rf freetype-2.9

wget $FREETYPE_URI

tar xvf freetype-2.9.tar.gz

cd freetype-2.9

mkdir _build && cd _build

cmake -G "Unix Makefiles" ../ "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true"  -DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=TRUE -DWITH_ZLIB=OFF -DWITH_BZIP2=OFF -DWITH_PNG=OFF

make

cd ../..

echo 'Freetype installation is finished.'


echo 'Installing ICU.'

rm -rf "${ICU_DIR_NAME}"

wget $ICU_URI

tar xvf "${ICU_FILENAME}"

cd "${ICU_DIR_NAME}/source" && mkdir lib && mkdir bin

chmod +x runConfigureICU configure install-sh

./runConfigureICU $ICU_CONFIG --enable-static --disable-shared

make

cd ../..

echo 'ICU installation is finished.'

echo 'Installing zlib and png.'

cd ..

cd libs/image/zlib/src && cmake "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true" && make && cd -
cd libs/image/png/src && cmake "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true" && make && cd -

echo 'zlib and png installation is finished.'

echo 'Copying libraries into their corresponding location.'
MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} == 'x86_64' ]; then
  ARCH="64"
else
  ARCH="32"
fi
cp libs/image/zlib/src/libzlib.a libs/image/zlib/$LIB_INSTALL/$ARCH/
cp libs/image/png/src/libpng.a libs/image/png/$LIB_INSTALL/$ARCH/
cp $DEP_INSTALL_DIR/freetype-2.9/_build/libfreetype.a libs/freetype/$LIB_INSTALL/$ARCH/
cp $DEP_INSTALL_DIR/libxml2-2.9.8/.libs/libxml2.a libs/libxml/$LIB_INSTALL/$ARCH/
if [[ "$OSTYPE" == "cygwin" ]]; then
mv $DEP_INSTALL_DIR/icu/source/lib/libsicuuc.a $DEP_INSTALL_DIR/icu/source/lib/libicuuc.a
mv $DEP_INSTALL_DIR/icu/source/lib/libsicudata.a $DEP_INSTALL_DIR/icu/source/lib/libicudata.a
fi
cp $DEP_INSTALL_DIR/icu/source/lib/libicuuc.a libs/icu/$LIB_INSTALL/$ARCH/
cp $DEP_INSTALL_DIR/icu/source/lib/libicudata.a libs/icu/$LIB_INSTALL/$ARCH/

rm -rf $DEP_INSTALL_DIR

echo 'installing additional language support packages.'
# see http://www.xpdfreader.com/download.html
mkdir -p languages && cd languages
languages=(
  "arabic"
  "chinese-simplified"
  "chinese-traditional"
  "cyrillic"
  "greek"
  "hebrew"
  "japanese"
  "korean"
  "latin2"
  "thai"
  "turkish"
)
for language in "${languages[@]}" ; do
  #language_dir="${language}"
  language_url="https://dl.xpdfreader.com/xpdf-${language}.tar.gz"
  wget -qO- "${language_url}"  | tar xvfz -  
done
cp xpdfrc ..

echo 'done.'
