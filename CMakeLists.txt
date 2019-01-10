cmake_minimum_required(VERSION 3.5+)
project(pdfalto)

set(CMAKE_CXX_STANDARD 11)

#build xpdf
set ( XPDF_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/xpdf-4.00)

set ( IMAGE_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image)

set ( PNG_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image/png)

set ( ZLIB_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/image/zlib)

set ( ICU_LIB_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu)

set(FREETYPE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/include)

if (APPLE)
    set ( FREETYPE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/mac/libfreetype.a)
    set ( ICUUC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/mac/libicuuc.a)
    set ( ICUDATA_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/mac/libicudata.a)
elseif (UNIX)
    set ( FREETYPE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/linux/libfreetype.a)
    set ( ICUUC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/linux/libicuuc.a)
    set ( ICUDATA_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/linux/libicudata.a)
elseif (MSVC)
    set ( FREETYPE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/windows/libfreetype.a)
    set ( ICUUC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/windows/libicuuc.a)
    set ( ICUDATA_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/icu/windows/libicudata.a)
endif ()


set(XPDF_BUILD_DIR ${XPDF_SUBDIR}/build)

include_directories("libs/image/png")

add_subdirectory(libs/image/zlib)
add_subdirectory(libs/image/png)

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
    target_link_libraries(pdfalto ${FREETYPE_LIBRARY} png xml2 xpdf goo fofi splash ${ICUUC_LIB} ${ICUDATA_LIB} dl ${HAVE_PAPER_H})
else()
    target_link_libraries(pdfalto ${FREETYPE_LIBRARY} png xml2 xpdf goo fofi splash ${ICUUC_LIB} ${ICUDATA_LIB} dl)
endif()

target_include_directories(pdfalto
        PUBLIC /usr/include/libxml2
        PUBLIC ${FREETYPE_INCLUDE_DIR}
        PUBLIC libs/image/png
        PUBLIC libs/image/zlib
        PUBLIC ${XPDF_SUBDIR}
        PUBLIC ${XPDF_SUBDIR}/goo
        PUBLIC ${XPDF_SUBDIR}/fofi
        PUBLIC ${XPDF_SUBDIR}/xpdf
        PUBLIC ${XPDF_BUILD_DIR}
        PUBLIC ${ICU_PATH}/common
        )