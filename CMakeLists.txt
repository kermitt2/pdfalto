cmake_minimum_required(VERSION 3.10+)
project(pdfalto VERSION 0.6.2)

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

set(XML_LIBRARY ${LIBXML_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libxml2.a)
set(PNG_LIBRARY ${PNG_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libpng.a)
set(ZLIB_LIBRARY ${ZLIB_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libz.a)
set(FREETYPE_LIBRARY ${FREETYPE_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libfreetype.a)
set(ICUUC_LIBRARY ${ICU_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libicuuc.a)
set(ICUDATA_LIBRARY ${ICU_SUBDIR}/${OSSUFFIX}/${ARCHSUFFIX}/libicudata.a)


set(XPDF_BUILD_DIR ${XPDF_SUBDIR}/build)

# xpdf generates aconf.h with configure_file() into the binary directory of its
# own subproject. For an in-source build (`cmake ./`) that is the source
# directory, which is why ${XPDF_SUBDIR} alone used to be enough; an
# out-of-source build -- what the Python wheel does -- puts it here instead.
set(XPDF_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/xpdf-4.05)

add_subdirectory(${XPDF_SUBDIR})

set(SOURCE_FILES
        src/compat_isoc23.c
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
        PUBLIC ${XPDF_BINARY_DIR}
        PUBLIC ${ICU_SUBDIR}/include
)

#--- install layout for the Python wheel (driven by scikit-build-core)
#
# Off by default so a plain `cmake ./ && make` keeps behaving exactly as before.
#
# The executable goes into the environment's bin/ so it lands on PATH, and its
# runtime resources into ../share/pdfalto. pdfalto resolves them relative to
# its own location (see findResourceDir in src/pdfalto.cc), and every install
# scheme pip uses -- venv, system, --user -- puts scripts in <prefix>/bin and
# data under <prefix>, so ../share/pdfalto resolves correctly in all of them.
option(PDFALTO_PYTHON_WHEEL "Install pdfalto into the layout expected by the Python wheel" OFF)

if (PDFALTO_PYTHON_WHEEL)
    # scikit-build-core defines these; the fallbacks make a manual
    # `cmake --install` produce the same standard prefix layout.
    if (NOT DEFINED SKBUILD_SCRIPTS_DIR)
        set(SKBUILD_SCRIPTS_DIR "bin")
    endif ()
    if (NOT DEFINED SKBUILD_DATA_DIR)
        set(SKBUILD_DATA_DIR ".")
    endif ()

    # Everything is tagged with a component so the wheel build can install only
    # these rules: the vendored xpdf tree installs libxpdf.a and a man page
    # unconditionally, which have no business inside a Python wheel.
    set(PDFALTO_WHEEL_COMPONENT "pdfalto_wheel")
    set(PDFALTO_WHEEL_DATA_DIR "${SKBUILD_DATA_DIR}/share/pdfalto")

    install(TARGETS pdfalto
            RUNTIME DESTINATION "${SKBUILD_SCRIPTS_DIR}"
            COMPONENT "${PDFALTO_WHEEL_COMPONENT}")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/xpdfrc"
            DESTINATION "${PDFALTO_WHEEL_DATA_DIR}"
            COMPONENT "${PDFALTO_WHEEL_COMPONENT}")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/languages"
            DESTINATION "${PDFALTO_WHEEL_DATA_DIR}"
            COMPONENT "${PDFALTO_WHEEL_COMPONENT}")
endif ()
