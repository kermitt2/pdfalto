cmake_minimum_required(VERSION 3.5+)
project(pdfalto)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "-no-pie")
set(CMAKE_BUILD_TYPE "Release")

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
set(XPDF_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/xpdf-4.03)

set(IMAGE_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image)

set(PNG_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image/png)

set(ZLIB_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image/zlib)

set(ICU_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu)

set(LIBXML_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/libxml)

set(FREETYPE_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype)

set(FREETYPE_INCLUDE_DIR_ft2build ${FREETYPE_SUBDIR}/include)
set(FREETYPE_INCLUDE_DIR_freetype_freetype ${FREETYPE_SUBDIR}/include)
set(FREETYPE_INCLUDE_DIR ${FREETYPE_SUBDIR}/include)

set(ZLIB_INCLUDE_DIRS ${ZLIB_SUBDIR}/include)
set(PNG_INCLUDE_DIRS ${PNG_SUBDIR}/include)
set(PNG_FOUND TRUE)

if (APPLE)
    message("Using macos settings.")
    #set(CMAKE_CXX_STANDARD_LIBRARIES "-static-compiler-rt -static-libc++")
    set(OSSUFFIX "mac")

    include(CheckCCompilerFlag)
    set(CMAKE_REQUIRED_LINK_OPTIONS "-arch;arm64")
    check_c_compiler_flag("-arch arm64" arm64Supported)
    message("arm64Supported=${arm64Supported}")


    IF (${arm64Supported} MATCHES 1)
        set(ARCHSUFFIX "arm64")
        MESSAGE(STATUS "Check if the building for macOS ARM - TRUE")
    ELSE ()
        set(ARCHSUFFIX "64")
        MESSAGE(STATUS "Check if the building for macOS ARM - FALSE")
    ENDIF ()

elseif (CYGWIN OR MSVC OR WIN32 OR MINGW)
    message("Using windows settings.")
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++")
    set(OSSUFFIX "windows")
elseif (UNIX)
    message("Using linux settings.")
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++")
    set(OSSUFFIX "linux")
endif ()

if (${OSSUFFIX} MATCHES "windows" OR ${OSSUFFIX} MATCHES "linux")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(ARM64 TRUE)
    else ()
        set(ARM64 FALSE)
    endif ()

    if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        if(ARM64)
            MESSAGE("++ 64 bit ARM architecture")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a")
            set(ARCHSUFFIX "arm64")
        else()
            MESSAGE("++ 64 bit architecture")
            set(ARCHSUFFIX "64")
        endif()
    else ()
        MESSAGE("++ 32 bit architecture")
        set(ARCHSUFFIX "32")
    endif ()
endif ()

MESSAGE("Final os/arch: " ${OSSUFFIX}/${ARCHSUFFIX})

set(XML_LIBRARY ${LIBXML_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libxml2.a)
set(PNG_LIBRARY ${PNG_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libpng.a)
set(ZLIB_LIBRARY ${ZLIB_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libz.a)
set(FREETYPE_LIBRARY ${FREETYPE_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libfreetype.a)
set(ICUUC_LIBRARY ${ICU_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libicuuc.a)
set(ICUDATA_LIBRARY ${ICU_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libicudata.a)


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

if (HAVE_PAPER_H)
    if (HAVE_FONTCONFIG)
        target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIBRARY} ${ICUDATA_LIBRARY} ${FREETYPE_LIBRARY} dl ${PAPER_LIBRARY} ${FONTCONFIG_LIBRARY} pthread)
    else ()
        target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIBRARY} ${ICUDATA_LIBRARY} ${FREETYPE_LIBRARY} dl ${PAPER_LIBRARY} pthread)
    endif ()
else ()
    if (HAVE_FONTCONFIG)
        target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIBRARY} ${ICUDATA_LIBRARY} ${FREETYPE_LIBRARY} dl ${FONTCONFIG_LIBRARY} pthread)
    else ()
        target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIBRARY} ${ICUDATA_LIBRARY} ${FREETYPE_LIBRARY} dl pthread)
    endif ()
endif ()

target_include_directories(
        pdfalto
        PUBLIC ${LIBXML_SUBDIR}/include
        PUBLIC ${FREETYPE_INCLUDE_DIR}
        PUBLIC ${PNG_INCLUDE_DIRS}
        PUBLIC ${ZLIB_INCLUDE_DIRS}
        PUBLIC ${XPDF_SUBDIR}
        PUBLIC ${XPDF_SUBDIR}/goo
        PUBLIC ${XPDF_SUBDIR}/fofi
        PUBLIC ${XPDF_SUBDIR}/xpdf
        PUBLIC ${XPDF_BUILD_DIR}
        PUBLIC ${ICU_SUBDIR}/include
)
