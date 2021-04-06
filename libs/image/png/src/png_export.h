
#ifndef PNG_EXPORT_H
#define PNG_EXPORT_H

#ifdef PNG_STATIC_DEFINE
#  define PNG_EXPORT
#  define PNG_NO_EXPORT
#else
#  ifndef PNG_EXPORT
#    ifdef png_EXPORTS
        /* We are building this library */
#      define PNG_EXPORT 
#    else
        /* We are using this library */
#      define PNG_EXPORT 
#    endif
#  endif

#  ifndef PNG_NO_EXPORT
#    define PNG_NO_EXPORT 
#  endif
#endif

#ifndef PNG_DEPRECATED
#  define PNG_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef PNG_DEPRECATED_EXPORT
#  define PNG_DEPRECATED_EXPORT PNG_EXPORT PNG_DEPRECATED
#endif

#ifndef PNG_DEPRECATED_NO_EXPORT
#  define PNG_DEPRECATED_NO_EXPORT PNG_NO_EXPORT PNG_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define PNG_NO_DEPRECATED
#endif

#endif
