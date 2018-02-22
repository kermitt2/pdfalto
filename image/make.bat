
set DIRPNG=.\png
set DIRZLIB=.\zlib
set CC=cl.exe
set CFLAGS= /I %DIRZLIB%
set LIBPROG=lib.exe


%CC% %CFLAGS%  -c %DIRZLIB%\adler32.c
%CC% %CFLAGS%  -c %DIRZLIB%\compress.c
%CC% %CFLAGS%  -c %DIRZLIB%\crc32.c
%CC% %CFLAGS%  -c %DIRZLIB%\deflate.c
%CC% %CFLAGS%  -c %DIRZLIB%\gzio.c
%CC% %CFLAGS%  -c %DIRZLIB%\infback.c
%CC% %CFLAGS%  -c %DIRZLIB%\inffast.c
%CC% %CFLAGS%  -c %DIRZLIB%\inflate.c
%CC% %CFLAGS%  -c %DIRZLIB%\inftrees.c
%CC% %CFLAGS%  -c %DIRZLIB%\trees.c
%CC% %CFLAGS%  -c %DIRZLIB%\uncompr.c
%CC% %CFLAGS%  -c %DIRZLIB%\zutil.c

del %DIRZLIB%\zlib.lib
%LIBPROG% /OUT:%DIRZLIB%\zlib.lib *.obj

%CC% %CFLAGS%  -c %DIRPNG%\png.c
%CC% %CFLAGS%  -c %DIRPNG%\pngerror.c
%CC% %CFLAGS%  -c %DIRPNG%\pnggccrd.c
%CC% %CFLAGS%  -c %DIRPNG%\pngget.c
%CC% %CFLAGS%  -c %DIRPNG%\pngmem.c
%CC% %CFLAGS%  -c %DIRPNG%\pngread.c
%CC% %CFLAGS%  -c %DIRPNG%\pngrio.c
%CC% %CFLAGS%  -c %DIRPNG%\pngrtran.c
%CC% %CFLAGS%  -c %DIRPNG%\pngrutil.c
%CC% %CFLAGS%  -c %DIRPNG%\pngset.c
%CC% %CFLAGS%  -c %DIRPNG%\pngtrans.c
%CC% %CFLAGS%  -c %DIRPNG%\pngvcrd.c
%CC% %CFLAGS%  -c %DIRPNG%\pngwio.c
%CC% %CFLAGS%  -c %DIRPNG%\pngwrite.c
%CC% %CFLAGS%  -c %DIRPNG%\pngwtran.c
%CC% %CFLAGS%  -c %DIRPNG%\pngwutil.c


del %DIRPNG%\libpng.lib
%LIBPROG% /OUT:%DIRPNG%\libpng.lib *.obj

