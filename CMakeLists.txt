cmake_minimum_required(VERSION 3.5+)
project(pdfalto)

set(CMAKE_CXX_STANDARD 11)

#build xpdf
set ( XPDF_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/xpdf-4.00)

set ( IMAGE_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image)

set ( PNG_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image/png)

set ( ZLIB_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image/zlib)

set ( ICU_LIB_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu)

set ( XML_LIB_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/libxml)

set(FREETYPE_INCLUDE_DIR_ft2build ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/include)
set(FREETYPE_INCLUDE_DIR_freetype_freetype ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/include)

set ( ICU_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu)

if (APPLE)
    message("Using macos settings.")
    set ( XML_LIBRARY ${XML_LIB_SUBDIR}/mac/libxml2.a)
    set ( PNG_LIBRARY ${PNG_SUBDIR}/mac/libpng.a)
    set ( ZLIB_LIBRARY ${ZLIB_SUBDIR}/mac/libzlib.a)
    set ( FREETYPE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/mac/libfreetype.a)
    set ( ICUUC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/mac/libicuuc.a)
    set ( ICUDATA_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/mac/libicudata.a)
elseif (CYGWIN OR MSVC OR WIN32 OR MINGW)
    message("Using windows settings.")
    set ( XML_LIBRARY ${XML_LIB_SUBDIR}/windows/libxml2.a)
    set ( PNG_LIBRARY ${PNG_SUBDIR}/windows/libpng.a)
    set ( ZLIB_LIBRARY ${ZLIB_SUBDIR}/windows/libzlib.a)
    set ( FREETYPE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/windows/libfreetype.a)
    set ( ICUUC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/windows/libsicuuc.a)
    set ( ICUDATA_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/windows/libsicudata.a)
elseif (UNIX)
    message("Using linux settings.")
    set ( XML_LIBRARY ${XML_LIB_SUBDIR}/linux/libxml2.a)
    set ( PNG_LIBRARY ${PNG_SUBDIR}/linux/libpng.a)
    set ( ZLIB_LIBRARY ${ZLIB_SUBDIR}/linux/libzlib.a)
    set ( FREETYPE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/linux/libfreetype.a)
    set ( ICUUC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/linux/libicuuc.a)
    set ( ICUDATA_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/linux/libicudata.a)
endif ()


set(XPDF_BUILD_DIR ${XPDF_SUBDIR}/build)

add_subdirectory(${XPDF_SUBDIR})

set(SOURCE_FILES
        src/AnnotsXrce.cc
        src/AnnotsXrce.h
        src/ConstantsUtils.cc
        src/ConstantsUtils.h
        src/ConstantsXMLALTO.cc
        src/ConstantsXMLALTO.h
        src/Parameters.cc
        src/Parameters.h
        src/PDFDocXrce.cc
        src/PDFDocXrce.h
        src/pdfalto.cc
        src/XmlAltoOutputDev.cc
        src/XmlAltoOutputDev.h
        )

add_executable(pdfalto ${SOURCE_FILES})

if(${HAVE_PAPER_H})
    target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIB} ${ICUDATA_LIB} ${FREETYPE_LIBRARY} dl ${HAVE_PAPER_H})
else()
    target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIB} ${ICUDATA_LIB} ${FREETYPE_LIBRARY} dl)
endif()

target_include_directories(pdfalto
        PUBLIC ${XML_LIB_SUBDIR}/include
        PUBLIC ${FREETYPE_INCLUDE_DIR_ft2build}
        PUBLIC ${PNG_SUBDIR}/src
        PUBLIC ${ZLIB_SUBDIR}/src
        PUBLIC ${XPDF_SUBDIR}
        PUBLIC ${XPDF_SUBDIR}/goo
        PUBLIC ${XPDF_SUBDIR}/fofi
        PUBLIC ${XPDF_SUBDIR}/xpdf
        PUBLIC ${XPDF_BUILD_DIR}
        PUBLIC ${ICU_PATH}/common
        )