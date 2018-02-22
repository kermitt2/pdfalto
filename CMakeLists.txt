cmake_minimum_required(VERSION 3.5+)
project(pdfalto)

set(CMAKE_CXX_STANDARD 98)

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

include(ExternalProject)
ExternalProject_Add(xpdf
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/xpdf-4.00
        CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/xpdf/configure --prefix=<INSTALL_DIR>
        BUILD_COMMAND ${MAKE})

set(XPDF_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/xpdf-4.00/build)

add_executable(pdfalto ${SOURCE_FILES})
add_dependencies(pdfalto xpdf)

target_link_libraries(pdfalto png zlib xml2 ${XPDF_BUILD_DIR}/lib/libxpdf.a ${XPDF_BUILD_DIR}/fofi/libfofi.a ${XPDF_BUILD_DIR}/goo/libGoo.a)
target_include_directories(pdfalto
        PUBLIC /usr/include/libxml2
        PUBLIC image/png
        PUBLIC xpdf
        PUBLIC xpdf/goo
        PUBLIC xpdf/fofi
        PUBLIC xpdf/xpdf
        PUBLIC ${XPDF_BUILD_DIR}
        )

add_subdirectory(image/png)
add_subdirectory(image/zlib)
