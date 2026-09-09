/*
 * glibc 2.38 added C23-conformant strtol/strtoul and, when <stdlib.h> is used
 * in a C23-aware mode, redirects strtol/strtoul to __isoc23_strtol and
 * __isoc23_strtoul via an asm label. Object code compiled that way therefore
 * carries references to symbols that older glibc does not export.
 *
 * The prebuilt aarch64 archives under libs/ -- libfreetype.a and libicuuc.a --
 * were built on a glibc 2.38+ host and reference them; the x86_64 archives,
 * built on an older base, do not. Linking the aarch64 ones inside the
 * manylinux image the wheels are built in (glibc 2.28) leaves those symbols
 * undefined. See CHANGELOG.md; the real fix is to rebuild those archives on a
 * base matching the x86_64 ones, at which point this file can be deleted.
 *
 * The two variants differ only in that the C23 ones accept a 0b/0B binary
 * prefix for base 0 and 2. Neither freetype nor ICU parses binary literals
 * here, so forwarding to the classic functions is behaviour-preserving in
 * practice.
 *
 * The definitions appear only when the C library does not already provide
 * them. That is also what makes them safe: on a glibc old enough to be missing
 * __isoc23_strtol, plain strtol is not redirected, so the wrappers below call
 * the real thing rather than recursing into themselves.
 */

/* <stdlib.h> rather than <features.h>: it declares the functions wrapped
 * below, and on glibc it pulls in <features.h> transitively, which is what
 * defines __GLIBC__ and __GLIBC_PREREQ. <features.h> is glibc's own header and
 * does not exist on macOS. */
#include <stdlib.h>

#if defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#if !__GLIBC_PREREQ(2, 38)
#define PDFALTO_NEEDS_ISOC23_SHIM 1
#endif
#endif

#ifdef PDFALTO_NEEDS_ISOC23_SHIM

long int __isoc23_strtol(const char *nptr, char **endptr, int base) {
    return strtol(nptr, endptr, base);
}

unsigned long int __isoc23_strtoul(const char *nptr, char **endptr, int base) {
    return strtoul(nptr, endptr, base);
}

long long int __isoc23_strtoll(const char *nptr, char **endptr, int base) {
    return strtoll(nptr, endptr, base);
}

unsigned long long int __isoc23_strtoull(const char *nptr, char **endptr,
                                         int base) {
    return strtoull(nptr, endptr, base);
}

#else

/* Keep the translation unit non-empty where the shims are not needed. */
typedef int pdfalto_compat_isoc23_not_needed;

#endif
