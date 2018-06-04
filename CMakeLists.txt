cmake_minimum_required(VERSION 3.5+)
project(pdfalto)

set(CMAKE_CXX_STANDARD 11)

#build xpdf
set ( XPDF_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/xpdf-4.00)

set(XPDF_BUILD_DIR ${XPDF_SUBDIR}/build)

add_subdirectory(image/png)
add_subdirectory(image/zlib)
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

target_link_libraries(pdfalto png zlib xml2 xpdf goo fofi ${XPDF_BUILD_DIR}/xpdf/lib/libxpdf.a)
target_include_directories(pdfalto
        PUBLIC /usr/include/libxml2
        PUBLIC image/png
        PUBLIC image/zlib
        PUBLIC ${XPDF_SUBDIR}
        PUBLIC ${XPDF_SUBDIR}/goo
        PUBLIC ${XPDF_SUBDIR}/fofi
        PUBLIC ${XPDF_SUBDIR}/xpdf
        PUBLIC ${XPDF_BUILD_DIR}
        )
