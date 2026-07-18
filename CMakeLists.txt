cmake_minimum_required(VERSION 3.10+)
project(pdfalto VERSION 0.6.0)

set(PDFALTO_VERSION_STR "${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "-no-pie")
set(CMAKE_BUILD_TYPE "Release")

# Set the SDK path
set(CMAKE_OSX_SYSROOT /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk)

#--- look for fontconfig
if (NOT NO_FONTCONFIG)
    find_library(FONTCONFIG_LIBRARY
            NAMES fontconfig libfontconfig
            PATH_SUFFIXES lib64 lib
    )
    if (FONTCONFIG_LIBRARY)
        set(HAVE_FONTCONFIG TRUE)
        message(STATUS "Found fontconfig at ${FONTCONFIG_LIBRARY}")
    else ()
        set(HAVE_FONTCONFIG FALSE)
        set(FONTCONFIG_LIBRARY "")
    endif ()
else ()
    set(HAVE_FONTCONFIG FALSE)
    set(FONTCONFIG_LIBRARY "")
endif ()

#build xpdf
set ( XPDF_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/xpdf-4.05)

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
    check_c_compiler_flag("-arch arm64" arm64Supported)
    message("arm64Supported=${arm64Supported}")


    IF (${arm64Supported} MATCHES 1)
        set(ARCHSUFFIX "arm64")
        set(CMAKE_REQUIRED_LINK_OPTIONS "-arch;arm64")
        MESSAGE(STATUS "Check if the building for MacOS ARM64")
    ELSE ()
        set(ARCHSUFFIX "64")
        set(C_INCLUDE_PATH "/usr/local/include")
        set(CPLUS_INCLUDE_PATH "/usr/local/include")
        MESSAGE(STATUS "Check if the building for MacOS X86_64")
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

# ── WINDOWS/MinGW SUPPORT: find system libraries via pkg-config ──────────────
#
# What this block does:
#   When building on Windows with MinGW (a toolchain that lets you compile
#   Linux-style C/C++ code on Windows), this block finds the required libraries
#   (libxml2, libpng, zlib, freetype, ICU) using the system's installed copies
#   instead of the pre-built static library files (.a files) that are bundled
#   ("vendored") in the pdfalto source tree.
#
# Why MinGW needs different treatment:
#   The vendored static libraries were compiled for Linux and Mac. They cannot
#   be linked into a Windows executable. Additionally, the vendored ICU header
#   files trigger compile errors with GCC 15+ (the version of the C++ compiler
#   shipped with modern MinGW). By using the system-installed versions (from
#   MSYS2's package manager), we get headers and libraries that are already
#   built for Windows and compatible with the current compiler.
#
# What pkg-config is:
#   pkg-config is a helper tool that answers the question "where are library X's
#   header files and compiled code on this system?" When you install a library
#   through a package manager (like MSYS2's pacman on Windows, or apt on Linux),
#   it registers a small ".pc" file that lists the include paths, library paths,
#   and linker flags. The commands below ask pkg-config for that information.
#
# What each CMake command does:
#   find_package(PkgConfig REQUIRED)
#     -- Tells CMake to locate the pkg-config tool itself. REQUIRED means
#        "stop with an error if pkg-config is not installed."
#
#   pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)
#     -- Asks pkg-config: "Is libxml-2.0 installed? If so, store its include
#        paths in LIBXML2_INCLUDE_DIRS, its library names in LIBXML2_LIBRARIES,
#        and its library folder paths in LIBXML2_LIBRARY_DIRS." REQUIRED means
#        "stop with an error if the library is not found."
#     -- The same pattern repeats for libpng, zlib, freetype2, and icu-uc.
#
#   set(XML_LIBRARY ${LIBXML2_LIBRARIES})
#     -- Stores the library name(s) that pkg-config found into the variable
#        XML_LIBRARY, which the rest of this CMake file uses when linking.
#
#   include_directories(...)
#     -- Tells the compiler "also look in these folders for #include header
#        files." Without this, the compiler would not find <libxml/parser.h>,
#        <unicode/normalizer2.h>, etc.
#
#   link_directories(...)
#     -- Tells the linker "also look in these folders for the compiled library
#        files." Without this, the linker would not find the .dll.a or .a files
#        that contain the actual compiled code.
#
# The "else" branch (non-MinGW):
#   On Linux and Mac, the vendored static .a files in the source tree are used
#   directly — no pkg-config needed. These are pre-built for each platform.
# ─────────────────────────────────────────────────────────────────────────────
if (MINGW)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)
    pkg_check_modules(LIBPNG REQUIRED libpng)
    pkg_check_modules(ZLIB REQUIRED zlib)
    pkg_check_modules(FREETYPE2 REQUIRED freetype2)
    pkg_check_modules(ICU REQUIRED icu-uc)
    set(XML_LIBRARY ${LIBXML2_LIBRARIES})
    set(PNG_LIBRARY ${LIBPNG_LIBRARIES})
    set(ZLIB_LIBRARY ${ZLIB_LIBRARIES})
    set(FREETYPE_LIBRARY ${FREETYPE2_LIBRARIES})
    set(ICUUC_LIBRARY ${ICU_LIBRARIES})
    set(ICUDATA_LIBRARY "")
    include_directories(${LIBXML2_INCLUDE_DIRS} ${ICU_INCLUDE_DIRS} ${FREETYPE2_INCLUDE_DIRS})
    link_directories(${LIBXML2_LIBRARY_DIRS} ${ICU_LIBRARY_DIRS} ${FREETYPE2_LIBRARY_DIRS})
else ()
    set(XML_LIBRARY ${LIBXML_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libxml2.a)
    set(PNG_LIBRARY ${PNG_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libpng.a)
    set(ZLIB_LIBRARY ${ZLIB_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libz.a)
    set(FREETYPE_LIBRARY ${FREETYPE_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libfreetype.a)
    set(ICUUC_LIBRARY ${ICU_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libicuuc.a)
    set(ICUDATA_LIBRARY ${ICU_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libicudata.a)
endif ()


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

target_compile_definitions(pdfalto PRIVATE PDFALTO_VERSION_STR=\"${PDFALTO_VERSION_STR}\")

if (HAVE_PAPER_H)
    if (HAVE_FONTCONFIG)
        target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIBRARY} ${ICUDATA_LIBRARY} ${FREETYPE_LIBRARY} dl ${PAPER_LIBRARY} ${FONTCONFIG_LIBRARY} pthread)
    else ()
        target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIBRARY} ${ICUDATA_LIBRARY} ${FREETYPE_LIBRARY} dl ${PAPER_LIBRARY} pthread)
    endif ()
else ()
    if (MINGW)
        # ── WINDOWS/MinGW SUPPORT: Windows-specific link libraries ───────────
        #
        # What target_link_libraries does:
        #   It tells the linker (the tool that combines compiled code into a
        #   final executable) which libraries the program depends on. Each name
        #   listed here is a library whose compiled code will be merged into
        #   (or referenced by) the pdfalto.exe output.
        #
        # Why MinGW uses different system libraries than Linux:
        #   On Linux, programs use "dl" (dynamic loader — for loading shared
        #   libraries at runtime) and "pthread" (POSIX threads — for running
        #   code in parallel). These are Unix concepts that do not exist on
        #   Windows. Instead, Windows has its own system libraries:
        #
        #   ole32  -- Provides COM (Component Object Model), which is Windows'
        #             system for letting programs share code and data. Some
        #             Windows APIs that libxml2 uses internally require COM.
        #
        #   uuid   -- Provides functions for working with UUIDs (Universally
        #             Unique Identifiers) — 128-bit labels used throughout
        #             Windows to identify COM objects, file types, etc.
        #
        #   ws2_32 -- The Windows Sockets 2 library, which provides network
        #             communication (TCP/IP). libxml2 can fetch XML schemas
        #             over the network, so it needs socket support even though
        #             pdfalto itself does not use networking.
        #
        # Note: "dl" and "pthread" are deliberately absent here — linking them
        # on Windows would cause errors because they do not exist.
        # ─────────────────────────────────────────────────────────────────────
        target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIBRARY} ${ICUDATA_LIBRARY} ${FREETYPE_LIBRARY} ole32 uuid ws2_32)
    elseif (HAVE_FONTCONFIG)
        target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIBRARY} ${ICUDATA_LIBRARY} ${FREETYPE_LIBRARY} dl ${FONTCONFIG_LIBRARY} pthread)
    else ()
        target_link_libraries(pdfalto ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${XML_LIBRARY} splash xpdf goo fofi ${ICUUC_LIBRARY} ${ICUDATA_LIBRARY} ${FREETYPE_LIBRARY} dl pthread)
    endif ()
endif ()

# ── WINDOWS/MinGW SUPPORT: reduced include directories ───────────────────────
#
# What target_include_directories does:
#   It tells the compiler "when the source code says #include <SomeFile.h>,
#   also search in these folders to find that file." Without this, the compiler
#   only searches its default system folders.
#
# Why MinGW has a shorter list:
#   On Linux/Mac (the "else" branch below), the compiler needs to be pointed at
#   ALL vendored library headers — libxml2, freetype, png, zlib, ICU — because
#   those headers are bundled in the source tree and are not installed on the
#   system.
#
#   On MinGW/Windows, those same libraries were installed via MSYS2's package
#   manager, and their headers are already in the system include path (added
#   earlier by the "include_directories" call in the pkg-config block above).
#   Pointing the compiler at the vendored copies as well would create
#   confusion: the compiler might pick the wrong version of a header, or —
#   worse — mix vendored headers with system-installed compiled code, causing
#   subtle build failures.
#
#   The xpdf headers (xpdf, goo, fofi) ARE still listed here because xpdf is
#   always built from the vendored source tree on every platform — it is not
#   available as a system package. The XPDF_BUILD_DIR entry points to the
#   generated file aconf.h (created by CMake during the xpdf build step), which
#   contains platform-specific configuration settings.
# ─────────────────────────────────────────────────────────────────────────────
if (MINGW)
    # MinGW: system headers via pkg-config (already added via include_directories above)
    # Only add xpdf paths (vendored, no conflict) + generated aconf.h
    target_include_directories(
            pdfalto
            PUBLIC ${XPDF_SUBDIR}
            PUBLIC ${XPDF_SUBDIR}/goo
            PUBLIC ${XPDF_SUBDIR}/fofi
            PUBLIC ${XPDF_SUBDIR}/xpdf
            PUBLIC ${XPDF_BUILD_DIR}
    )
else ()
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
endif ()
