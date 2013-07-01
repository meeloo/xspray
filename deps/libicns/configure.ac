#         -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT(libicns, 0.8.1, mathew@eisbox.net)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/icns.h])
AM_MAINTAINER_MODE([enable])

# To enable debugging mode
CFLAGS="${CFLAGS=}"
AC_MSG_CHECKING(whether to enable debugging)
AC_ARG_ENABLE(debug, [  --enable-debug=[yes/no] turn on debugging [default=no]],, enable_debug=no)
if test "x$enable_debug" = "xyes"; then
         CFLAGS="$CFLAGS -g -DDEBUG -DICNS_DEBUG"
AC_MSG_RESULT(yes)
else
AC_MSG_RESULT(no)
fi

# Stuff for the config.h.in file
AC_CONFIG_HEADERS(config.h)
AC_GNU_SOURCE

# Checks for programs.
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(stdint.h)
AC_CHECK_HEADERS(getopt.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_MODE_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_LIB(getopt,getopt_long)

# Check for memcpy unaligned copy support
AC_MSG_CHECKING([whether memcpy works with unaligned data])
AC_RUN_IFELSE([
AC_LANG_PROGRAM([[#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define READ_UNALIGNED(val, addr, size)  memcpy(&(val), (addr), size)
typedef struct struct_type_t {
  uint32_t	numx;
  uint32_t	numy;
  uint8_t   data[1];
} struct_type_t;]],[[uint8_t	*init = "WithThisDataICanFindManyBugs\0\0\0\0";
uint8_t *data = malloc(sizeof(uint8_t)*32);
struct_type_t *good_data = (struct_type_t *)(data+0);
struct_type_t *poor_data = (struct_type_t *)(data+1);
uint32_t numx = 0;
uint32_t numy = 0;
uint8_t  ebytes[] = {0xFF,0x11};
uint16_t echeck = ebytes[0]|ebytes[1]<< 8;
uint16_t endian = *((uint16_t*)(&ebytes[0]));
uint8_t  little = (endian == echeck);
memcpy(&data[0],&init[0],32);
if(memcmp(&data[0],&init[0],32) != 0) return 1;
READ_UNALIGNED(numx, &(poor_data->numx),sizeof( uint32_t));
if( (little && numx != 0x54687469) || (!little && numx != 0x69746854) ) return 1;
READ_UNALIGNED(numy, &(poor_data->numy),sizeof( uint32_t));
if( (little && numy != 0x44736968) || (!little && numy != 0x68697344) ) return 1;
READ_UNALIGNED(numx, &(good_data->numx),sizeof( uint32_t));
if( (little && numx != 0x68746957) || (!little && numx != 0x57697468) ) return 1;
READ_UNALIGNED(numy, &(good_data->numy),sizeof( uint32_t));
if( (little && numy != 0x73696854) || (!little && numy != 0x54686973) ) return 1;
]])], [
AC_DEFINE([HAVE_UNALIGNED_MEMCPY],[1],[memcpy works with unaligned data])
AC_MSG_RESULT(yes)
], [
AC_DEFINE([HAVE_UNALIGNED_MEMCPY],[0],[memcpy fails with unaligned data])
AC_MSG_RESULT(no)
], [
AC_MSG_RESULT(can not test)
] )

# Check for noinline memcpy unaligned copy support
AC_MSG_CHECKING([whether noinline memcpy works with unaligned data])
AC_RUN_IFELSE([
AC_LANG_PROGRAM([[#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifndef HAVE_UNALIGNED_MEMCPY
 #if defined(__GNUC__) && (defined(__arm__) || defined(__thumb__) || defined(__sparc__))
  #define HAVE_UNALIGNED_MEMCPY 0
 #else
  #define HAVE_UNALIGNED_MEMCPY 1
 #endif
#endif
#if HAVE_UNALIGNED_MEMCPY == 0
__attribute__ ((noinline)) void *memcpy_noinline( void *dst, const void *src, size_t num );
__attribute__ ((noinline)) void *memcpy_noinline( void *dst, const void *src, size_t num ) {
 return memcpy(dst,src,num);
}
#define READ_UNALIGNED(val, addr, size)  memcpy_noinline(&(val), (addr), size)
#else
#define READ_UNALIGNED(val, addr, size)  memcpy(&(val), (addr), size)
#endif
typedef struct struct_type_t {
  uint32_t	numx;
  uint32_t	numy;
  uint8_t   data[1];
} struct_type_t;]],[[uint8_t	*init = "WithThisDataICanFindManyBugs\0\0\0\0";
uint8_t *data = malloc(sizeof(uint8_t)*32);
struct_type_t *good_data = (struct_type_t *)(data+0);
struct_type_t *poor_data = (struct_type_t *)(data+1);
uint32_t numx = 0;
uint32_t numy = 0;
uint8_t  ebytes[] = {0xFF,0x11};
uint16_t echeck = ebytes[0]|ebytes[1]<< 8;
uint16_t endian = *((uint16_t*)(&ebytes[0]));
uint8_t  little = (endian == echeck);
memcpy(&data[0],&init[0],32);
if(memcmp(&data[0],&init[0],32) != 0) return 1;
READ_UNALIGNED(numx, &(poor_data->numx),sizeof( uint32_t));
if( (little && numx != 0x54687469) || (!little && numx != 0x69746854) ) return 1;
READ_UNALIGNED(numy, &(poor_data->numy),sizeof( uint32_t));
if( (little && numy != 0x44736968) || (!little && numy != 0x68697344) ) return 1;
READ_UNALIGNED(numx, &(good_data->numx),sizeof( uint32_t));
if( (little && numx != 0x68746957) || (!little && numx != 0x57697468) ) return 1;
READ_UNALIGNED(numy, &(good_data->numy),sizeof( uint32_t));
if( (little && numy != 0x73696854) || (!little && numy != 0x54686973) ) return 1;
]])], [
AC_MSG_RESULT(yes)
], [
AC_MSG_RESULT(no)
AC_MSG_ERROR(libicns cannot work around memcpy problems on this platform)
], [
AC_MSG_RESULT(can not test)
] )

# Check for libpng
AC_CHECK_LIB(png, png_create_read_struct, [
AC_SUBST(PNG_LIBS, "-lpng")
], [
  AC_CHECK_LIB(png10, png_create_read_struct, [
  AC_SUBST(PNG_LIBS, "-lpng10")
  ], [
    AC_CHECK_LIB(png12, png_create_read_struct, [
    AC_SUBST(PNG_LIBS, "-lpng12")
    ], [
      AC_MSG_ERROR([cannot find required library png])
    ], [])
  ], [])
], [])
AC_CHECK_HEADERS([png.h libpng/png.h libpng10/png.h libpng12/png.h])

# Check for libopenjpeg, fall back to libjasper if not available
AC_CHECK_LIB(jasper, jas_init, [
AC_SUBST(JP2000_LIBS, "-ljasper")
AC_CHECK_HEADERS([jasper/jasper.h])
AC_DEFINE([ICNS_JASPER],[1],[We have Jasper])
], [
  AC_CHECK_LIB(openjpeg2, opj_setup_decoder, [
  AC_SUBST(JP2000_LIBS, "-lopenjpeg2")
  AC_CHECK_HEADERS([openjpeg.h])
  AC_DEFINE([ICNS_OPENJPEG],[1],[We have OpenJPEG])
  ], [
    AC_CHECK_LIB(openjpeg, opj_setup_decoder, [
    AC_SUBST(JP2000_LIBS, "-lopenjpeg")
    AC_CHECK_HEADERS([openjpeg.h])
    AC_DEFINE([ICNS_OPENJPEG],[1],[We have OpenJPEG])
    ], [
      AC_MSG_WARN([libopenjpeg or libjasper jp2000 codec libraries not found])
      AC_MSG_WARN([libicns will be built without 256x256 and 512x512 support])
    ])
  ])
])

AC_CONFIG_FILES([Makefile libicns.spec icnsutils/Makefile src/Makefile src/libicns.pc])
AC_OUTPUT

