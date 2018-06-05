cmake_minimum_required(VERSION 3.5+)
project(zlib)

set(CMAKE_CXX_STANDARD 98)

set(ZLIB_SOURCE_FILES
        adler32.c
        compress.c
        crc32.c
        crc32.h
        deflate.c
        deflate.h
        gzio.c
        infback.c
        inffast.c
        inffast.h
        inffixed.h
        inflate.c
        inflate.h
        inftrees.c
        inftrees.h
        trees.c
        trees.h
        uncompr.c
        zconf.h
        zconf.in.h
        zlib.h
        zutil.c
        zutil.h
        )

add_library(${PROJECT_NAME} STATIC ${ZLIB_SOURCE_FILES})
