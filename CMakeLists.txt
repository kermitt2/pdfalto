cmake_minimum_required(VERSION 3.5+)
project(pdfalto)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXE_LINKER_FLAGS "-no-pie")

#--- look for fontconfig
if (NOT NO_FONTCONFIG)
  find_library(FONTCONFIG_LIBRARY
               NAMES fontconfig libfontconfig
               PATH_SUFFIXES lib64 lib
  )
  if (FONTCONFIG_LIBRARY)
    set(HAVE_FONTCONFIG TRUE)
    message(STATUS "Found fontconfig")
  else ()
    set(HAVE_FONTCONFIG FALSE)
    set(FONTCONFIG_LIBRARY "")
  endif ()
else ()
  set(HAVE_FONTCONFIG FALSE)
  set(FONTCONFIG_LIBRARY "")
endif ()

#build xpdf
set ( XPDF_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/xpdf-4.03)

set ( IMAGE_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image)

set ( PNG_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image/png)

set ( ZLIB_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image/zlib)

set ( ICU_LIB_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu)

set ( XML_LIB_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/libxml)

set(FREETYPE_INCLUDE_DIR_ft2build ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/include)
set(FREETYPE_INCLUDE_DIR_freetype_freetype ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/include)

include_directories("${ZLIB_SUBDIR}/src")
set(PNG_INCLUDE_DIRS ${PNG_SUBDIR}/src)
set(PNG_FOUND TRUE)

set ( ICU_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu)

if (APPLE)
    message("Using macos settings.")
    #set(CMAKE_CXX_STANDARD_LIBRARIES "-static-compiler-rt -static-libc++")
    set(OSSUFFIX "mac")
elseif (CYGWIN OR MSVC OR WIN32 OR MINGW)
    message("Using windows settings.")
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++")
    set(OSSUFFIX "windows")
elseif (UNIX)
    message("Using linux settings.")
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++")
    set(OSSUFFIX "linux")
endif ()

if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    MESSAGE("++ 64 bit architecture")
    set(ARCHSUFFIX "64")
else()
    MESSAGE("++ 32 bit architecture")
    set(ARCHSUFFIX "32")
endif()

set ( XML_LIBRARY ${XML_LIB_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libxml2.a)
set ( PNG_LIBRARIES ${PNG_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libpng.a)
set ( ZLIB_LIBRARY ${ZLIB_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libzlib.a)
set ( FREETYPE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/${OSSUFFIX}/${ARCHSUFFIX}/libfreetype.a)
set ( ICUUC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/${OSSUFFIX}/${ARCHSUFFIX}/libicuuc.a)
set ( ICUDATA_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/${OSSUFFIX}/${ARCHSUFFIX}/libicudata.a)


set(XPDF_BUILD_DIR ${XPDF_SUBDIR}/build)

add_subdirectory(${XPDF_SUBDIR})

set(SOURCE_FILES
        src/AnnotsXrce.cc
        src/AnnotsXrce.h
        src/ConstantsUtils.cc
        src/ConstantsUtils.h
        src/ConstantsXML.cc
        src/ConstantsXML.h
        src/Parameters.cc
        src/Parameters.h
        src/PDFDocXrce.cc
        src/PDFDocXrce.h
        src/whereami.c
        src/whereami.h
        src/pdfalto.cc
        src/XmlAltoOutputDev.cc
        src/XmlAltoOutputDev.h
        )

add_executable(pdfalto ${SOURCE_FILES})

if(HAVE_PAPER_H)
    if(HAVE_FONTCONFIG)
        target_link_libraries(pdfalto ${PNG_LIBRARIES} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIB} ${ICUDATA_LIB} ${FREETYPE_LIBRARY} dl ${PAPER_LIBRARY} ${FONTCONFIG_LIBRARY})
    else()
        target_link_libraries(pdfalto ${PNG_LIBRARIES} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIB} ${ICUDATA_LIB} ${FREETYPE_LIBRARY} dl ${PAPER_LIBRARY})
    endif()
else()
    if(HAVE_FONTCONFIG)
        target_link_libraries(pdfalto ${PNG_LIBRARIES} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIB} ${ICUDATA_LIB} ${FREETYPE_LIBRARY} dl ${FONTCONFIG_LIBRARY})
    else()
        target_link_libraries(pdfalto ${PNG_LIBRARIES} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIB} ${ICUDATA_LIB} ${FREETYPE_LIBRARY} dl)
    endif()
endif()

target_include_directories(pdfalto
        PUBLIC ${XML_LIB_SUBDIR}/include
        PUBLIC ${FREETYPE_INCLUDE_DIR_ft2build}
        PUBLIC ${PNG_INCLUDE_DIRS}
        PUBLIC ${ZLIB_SUBDIR}/src
        PUBLIC ${XPDF_SUBDIR}
        PUBLIC ${XPDF_SUBDIR}/goo
        PUBLIC ${XPDF_SUBDIR}/fofi
        PUBLIC ${XPDF_SUBDIR}/xpdf
        PUBLIC ${XPDF_BUILD_DIR}
        PUBLIC ${ICU_PATH}/common
        )
