cmake_minimum_required(VERSION 3.5+)
project(pdfalto)

set(CMAKE_CXX_STANDARD 98)

add_subdirectory(image/png)
add_subdirectory(image/zlib)

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

set(XPDF_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/xpdf-4.00/build)

add_executable(pdfalto ${SOURCE_FILES})
#add_dependencies(pdfalto xpdf-4.00)

target_link_libraries(pdfalto png zlib xml2 ${XPDF_BUILD_DIR}/xpdf/lib/libxpdf.a ${XPDF_BUILD_DIR}/fofi/libfofi.a ${XPDF_BUILD_DIR}/goo/libgoo.a)
target_include_directories(pdfalto
        PUBLIC /usr/include/libxml2
        PUBLIC image/png
        PUBLIC image/zlib
        PUBLIC xpdf-4.00/build
        PUBLIC xpdf-4.00
        PUBLIC xpdf-4.00/goo
        PUBLIC xpdf-4.00/fofi
        PUBLIC xpdf-4.00/xpdf
        PUBLIC ${XPDF_BUILD_DIR}
        )
