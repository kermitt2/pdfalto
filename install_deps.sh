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

LIBXML2_VERSION=2.13.6
FREETYPE2_VERSION=2.13.3
LIBPNG16_VERSION=1.6.47
ZLIB_VERSION=1.3.1

LIBXML_URI="https://gitlab.gnome.org/GNOME/libxml2/-/archive/v${LIBXML2_VERSION}/libxml2-v${LIBXML2_VERSION}.tar.gz"
FREETYPE_URI="https://sourceforge.net/projects/freetype/files/freetype2/${FREETYPE2_VERSION}/freetype-${FREETYPE2_VERSION}.tar.gz/download"
LIBPNG_URI="https://sourceforge.net/projects/libpng/files/libpng16/${LIBPNG16_VERSION}/libpng-${LIBPNG16_VERSION}.tar.gz/download"
ZLIB_URI="https://zlib.net/zlib-${ZLIB_VERSION}.tar.gz"
ICU_URI="https://github.com/unicode-org/icu/archive/refs/tags/release-76-1.tar.gz"

ICU_FILENAME=$(basename "${ICU_URI}")
ICU_DIR_NAME=icu

mkdir -p "${DEP_INSTALL_DIR}"

cd "${DEP_INSTALL_DIR}"

echo ${OSTYPE}

if [[ "${OSTYPE}" == "linux"* ]]; then
  if ! [ -x "$(command -v autoreconf)" ]; then
    sudo apt-get install autoconf
  fi
  ICU_CONFIG="Linux"
  LIB_INSTALL="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
  # Mac OSX
  if ! [ -x "$(command -v autoreconf)" ]; then
    brew install autoconf automake
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
else
  echo "Unknown OSTYPE: $OSTYPE, assuming it's LINUX"
  if ! [ -x "$(command -v autoreconf)" ]; then
    sudo apt-get install autoconf
  fi
  ICU_CONFIG="Linux"
  LIB_INSTALL="linux"
fi

echo "${LIB_INSTALL}"

echo 'Installing libxml2.'
#
rm -rf libxml2
wget -O libxml2.tar.gz -nc "${LIBXML_URI}"
tar -xzf libxml2.tar.gz
mv libxml2-* libxml2
cd libxml2
mkdir -p build/output
cd build
cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/output -G "Unix Makefiles" .. -D LIBXML2_WITH_LZMA=OFF -D LIBXML2_WITH_ICONV=OFF -D LIBXML2_WITH_ZLIB=OFF -DBUILD_SHARED_LIBS=OFF
make
make install
cd ../..

echo 'libxml2 installation is finished.'


echo 'Installing freetype.'

rm -rf freetype
wget -O freetype.tar.gz -nc "${FREETYPE_URI}"
tar -xzf freetype.tar.gz
mv freetype-* freetype
cd freetype
mkdir -p build/output
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/output -G "Unix Makefiles" -D FT_DISABLE_BROTLI=TRUE -D FT_DISABLE_HARFBUZZ=TRUE -D FT_DISABLE_ZLIB=TRUE -D FT_DISABLE_BZIP2=TRUE -D FT_DISABLE_PNG=TRUE
make
make install
cd ../..

echo 'Freetype installation is finished.'


echo 'Installing ICU.'

rm -rf "${ICU_DIR_NAME}"
wget -O "${ICU_FILENAME}" -nc "${ICU_URI}"
tar -xzf "${ICU_FILENAME}"
mv icu-release-* "${ICU_DIR_NAME}"
cd "${ICU_DIR_NAME}/icu4c/source"
mkdir output
chmod +x runConfigureICU configure install-sh
echo "ICU_CONFIG -> ${ICU_CONFIG}"
./runConfigureICU "${ICU_CONFIG}" --enable-static --disable-shared --prefix=$(pwd)/output
make
make install
cd ../../..

echo 'ICU installation is finished.'

echo 'Installing zlib and png.'

rm -rf zlib
wget -O zlib.tar.gz -nc "${ZLIB_URI}"
tar -xzf zlib.tar.gz
mv zlib-* zlib
cd zlib
mkdir -p build/output
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/output -DBUILD_SHARED_LIBS=OFF
make
make install
cd ../..

rm -rf libpng
wget -O libpng.tar.gz -nc "${LIBPNG_URI}"
tar -xzf libpng.tar.gz
mv libpng* libpng
cd libpng
mkdir -p build/output
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/output -DBUILD_SHARED_LIBS=OFF
make
make install
cd ../..

echo 'zlib and png installation is finished.'

echo 'Copying libraries into their corresponding location.'
MACHINE_TYPE=$(uname -m)
if [[ "${MACHINE_TYPE}" == 'x86_64' ]]; then
  ARCH="64"
elif [[ "${MACHINE_TYPE}" == 'arm64' ]]; then
  ARCH="arm64"
else
  ARCH="32"
fi

mkdir -p "libs/libxml/${LIB_INSTALL}/${ARCH}" "libs/freetype/${LIB_INSTALL}/${ARCH}" "libs/icu/${LIB_INSTALL}/${ARCH}" "libs/image/zlib/${LIB_INSTALL}/${ARCH}" "libs/image/png/${LIB_INSTALL}/${ARCH}"

cp "${DEP_INSTALL_DIR}/freetype/build/output/lib/libfreetype.a" "libs/freetype/${LIB_INSTALL}/${ARCH}/"
cp "${DEP_INSTALL_DIR}/libxml2/build/output/lib/libxml2.a" "libs/libxml/${LIB_INSTALL}/${ARCH}/"

cp "${DEP_INSTALL_DIR}/icu/icu4c/source/output/lib/libicuuc.a" "libs/icu/${LIB_INSTALL}/${ARCH}/"
cp "${DEP_INSTALL_DIR}/icu/icu4c/source/output/lib/libicudata.a" "libs/icu/${LIB_INSTALL}/${ARCH}/"

cp "${DEP_INSTALL_DIR}/zlib/build/output/lib/libz.a" "libs/image/zlib/${LIB_INSTALL}/$ARCH/"
cp "${DEP_INSTALL_DIR}/libpng/build/output/lib/libpng.a" "libs/image/png/${LIB_INSTALL}/$ARCH/"
cp "${DEP_INSTALL_DIR}/libpng/build/output/lib/libpng16.a" "libs/image/png/${LIB_INSTALL}/$ARCH/"

rm -rf libs/libxml/include
cp -R "${DEP_INSTALL_DIR}/libxml2/build/output/include/libxml2" libs/libxml
mv libs/libxml/libxml2 libs/libxml/include

rm -rf libs/freetype/include
cp -R "${DEP_INSTALL_DIR}/freetype/build/output/include/freetype2" libs/freetype
mv libs/freetype/freetype2 libs/freetype/include

rm -rf libs/icu/include
cp -R "${DEP_INSTALL_DIR}/icu/icu4c/source/output/include" libs/icu

rm -rf libs/image/zlib/include
cp -R "${DEP_INSTALL_DIR}/zlib/build/output/include" libs/image/zlib

rm -rf libs/image/png/include
cp -R "${DEP_INSTALL_DIR}/libpng/build/output/include" libs/image/png

cd ..
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
  wget -qO- "${language_url}" | tar xvfz -
done
cp xpdfrc ..

echo 'done.'
