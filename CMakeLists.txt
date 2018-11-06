cmake_minimum_required(VERSION 3.5+)
project(pdfalto)

set(CMAKE_CXX_STANDARD 11)

#build xpdf
set ( XPDF_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/xpdf-4.00)

set ( IMAGE_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/image)

set ( PNG_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/image/png)

set ( ZLIB_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/image/zlib)

set(XPDF_BUILD_DIR ${XPDF_SUBDIR}/build)

include_directories("image/zlib")
include_directories("image/png")

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

if(${HAVE_PAPER_H})
    target_link_libraries(pdfalto ${FREETYPE_LIBRARY} ${FREETYPE_OTHER_LIBS} zlib png xml2 xpdf goo fofi splash ${ICU_PATH}/lib/libicuuc.a ${ICU_PATH}/lib/libicudata.a dl ${HAVE_PAPER_H})
else()
    target_link_libraries(pdfalto ${FREETYPE_LIBRARY} ${FREETYPE_OTHER_LIBS} zlib png xml2 xpdf goo fofi splash ${ICU_PATH}/lib/libicuuc.a ${ICU_PATH}/lib/libicudata.a dl)
endif()

if(FREETYPE_INCLUDE_DIR_ft2build)
    set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR_ft2build})
elseif(FREETYPE_INCLUDE_DIR_freetype)
    set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR_freetype})
elseif(FREETYPE_INCLUDE_DIR_freetype_freetype)
    set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR_freetype_freetype})
endif()
message(STATUS "${FREETYPE_INCLUDE_DIR}")
target_include_directories(pdfalto
        PUBLIC /usr/local/opt/libxml2/include/libxml2
        PUBLIC ${FREETYPE_INCLUDE_DIR}
        PUBLIC image/png
        PUBLIC image/zlib
        PUBLIC ${XPDF_SUBDIR}
        PUBLIC ${XPDF_SUBDIR}/goo
        PUBLIC ${XPDF_SUBDIR}/fofi
        PUBLIC ${XPDF_SUBDIR}/xpdf
        PUBLIC ${XPDF_BUILD_DIR}
        PUBLIC ${ICU_PATH}/common
        )

# BUILD TRAIN GENERATOR
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
        src/traingenerator.cc
        src/DataGeneratorOutputDev.cc
        src/DataGeneratorOutputDev.h
        )

add_executable(traingenerator ${SOURCE_FILES})

if(${HAVE_PAPER_H})
    target_link_libraries(traingenerator ${FREETYPE_LIBRARY} ${FREETYPE_OTHER_LIBS} zlib png xml2 xpdf goo fofi splash dl ${HAVE_PAPER_H})
else()
    target_link_libraries(traingenerator ${FREETYPE_LIBRARY} ${FREETYPE_OTHER_LIBS} zlib png xml2 xpdf goo fofi splash dl)
endif()

if(FREETYPE_INCLUDE_DIR_ft2build)
    set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR_ft2build})
elseif(FREETYPE_INCLUDE_DIR_freetype)
    set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR_freetype})
elseif(FREETYPE_INCLUDE_DIR_freetype_freetype)
    set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR_freetype_freetype})
endif()
message(STATUS "${FREETYPE_INCLUDE_DIR}")
target_include_directories(traingenerator
        PUBLIC /usr/local/opt/libxml2/include/libxml2
        PUBLIC ${FREETYPE_INCLUDE_DIR}
        PUBLIC image/png
        PUBLIC image/zlib
        PUBLIC ${XPDF_SUBDIR}
        PUBLIC ${XPDF_SUBDIR}/goo
        PUBLIC ${XPDF_SUBDIR}/fofi
        PUBLIC ${XPDF_SUBDIR}/xpdf
        PUBLIC ${XPDF_BUILD_DIR}
        )