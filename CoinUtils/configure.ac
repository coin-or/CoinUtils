# Copyright (C) 2006,2009 International Business Machines.
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.

## $Id$

# Author:  Andreas Waechter            IBM    2006-04-13

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_INIT([CoinUtils],[trunk],[coinutils@lists.coin-or.org],[coinutils],
	[http://projects.coin-or.org/CoinUtils])
AC_COPYRIGHT([
Copyright 2006 International Business Machines and others.
All Rights Reserved.
This file is part of the open source package CoinUtils which is distributed
under the Eclipse Public License.])

# List one file in the package so that the configure script can test whether
# the package is actually there.

AC_CONFIG_SRCDIR(src/CoinError.cpp)

# Do some initialization work (version numbers, change prefix default, ...)

AC_COIN_INITIALIZE(CoinUtils)

#############################################################################
#                         Standard build tool stuff                         #
#############################################################################

# Get the name of the C++ compiler and appropriate compiler options

AC_COIN_PROG_CXX

# Sort out what we need to link to Fortran libraries.

AC_COIN_PROG_F77
if test "$F77" != "unavailable" ; then
  AC_COIN_F77_SETUP
else
  AC_COIN_F77_WRAPPERS(blas,DAXPY)
fi

# Initialize libtool

AC_COIN_PROG_LIBTOOL

############################################################################
#                   Stuff that we need for finite and isnan                #
############################################################################

AC_COIN_CHECK_ISFINITE
AC_COIN_CHECK_ISNAN

#############################################################################
#                         Thread configuration                              #
#############################################################################

# Define new options: 
#    --enable-coinutils-threads 
#    --enable-coinutils-mempool-override-new
#    --enable-coinutils-mempool-maxpooled

# Enable compilation of thread-aware features in CoinUtils
AC_COINUTILS_THREADS(yes)

# Activate the CoinUtils mempool for blocks up to 4096 bytes, but do not
# redirect global new/delete to the CoinUtils mempool.
# 170909  (lh) Changing the second parameter to yes causes compilation errors
#         in CoinAlloc with C++11. Likely the best solution is to remove the
#         dynamic throw spec.
AC_COINUTILS_MEMPOOL(4096,no)

#############################################################################
#                       Finding certain integer types                       #
#############################################################################

AC_LANG_PUSH(C++)
AC_CHECK_HEADER(inttypes)
AC_CHECK_HEADER(stdint)
AC_LANG_POP(C++)

##### 64bit Integer types
# The problem here is that you can't extend `int64_t' to `unsigned int64_t'.
# So we need distinct CoinInt64 and CoinUInt64. It should be safe to assume
# uint64_t, given int64_t.
CoinInt64=
CoinUInt64=

# try int64_t
AC_CHECK_TYPE([int64_t],[CoinInt64=int64_t ; CoinUInt64=uint64_t],[],[
#ifdef HAVE_CINTTYPES
# include <cinttypes>
#else
# ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
# endif
#endif
])

# We need to use the C compiler in the AC_CHECK_SIZEOF since otherwise the
# MSCV compiler complains about redefinition of "exit". ac_cv_sizeof_<type>
# sometimes adds `^M' to the number, hence the check for `8?'.
AC_LANG_PUSH(C)
# try long long
if test x"$CoinInt64" = x; then
  AC_CHECK_SIZEOF([long long])
  case $ac_cv_sizeof_long_long in
    8 | 8?) CoinInt64="long long"
            CoinUInt64="unsigned long long"
            ;;
  esac
fi

#try long
if test x"$CoinInt64" = x; then
  AC_CHECK_SIZEOF([long])
  case $ac_cv_sizeof_long in
    8 | 8?) CoinInt64="long"
            CoinUInt64="unsigned long"
            ;;
  esac
fi

#try int
if test x"$CoinInt64" = x; then
  AC_CHECK_SIZEOF([int])
  case $ac_cv_sizeof_int in
    8 | 8?) CoinInt64="int"
            CoinUInt64="unsigned int"
            ;;
  esac
fi
AC_LANG_POP(C)

if test x"$CoinInt64" = x; then
  AC_MSG_ERROR([Cannot find integer type with 64 bits])
fi
AC_DEFINE_UNQUOTED([COIN_INT64_T], [$CoinInt64],[Define to 64bit integer type])
AC_DEFINE_UNQUOTED([COIN_UINT64_T],[$CoinInt64],[Define to 64bit unsigned integer type])


##### Integer type for Pointer
CoinIntPtr=

# try intptr_t
AC_CHECK_TYPE([intptr_t],[CoinIntPtr=intptr_t],[],[
#ifdef HAVE_CINTTYPES
# include <cinttypes>
#else
# ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
# endif
#endif
])

AC_LANG_PUSH(C)
# try long long
if test x"$CoinIntPtr" = x; then
  AC_CHECK_SIZEOF([int *])
  AC_CHECK_SIZEOF([long long])
  if test "$ac_cv_sizeof_long_long" = "$ac_cv_sizeof_int_p"; then
    CoinIntPtr="long long"
  fi
fi

# try long
if test x"$CoinIntPtr" = x; then
  AC_CHECK_SIZEOF([long])
  if test "$ac_cv_sizeof_long" = "$ac_cv_sizeof_int_p"; then
    CoinIntPtr="long"
  fi
fi

# try int
if test x"$CoinIntPtr" = x; then
  AC_CHECK_SIZEOF([int])
  if test "$ac_cv_sizeof_int" = "$ac_cv_sizeof_int_p"; then
    CoinIntPtr="int"
  fi
fi
AC_LANG_POP(C)

if test x"$CoinIntPtr" = x; then
  AC_MSG_ERROR([Cannot find integer type capturing pointer])
fi
AC_DEFINE_UNQUOTED([COIN_INTPTR_T],[$CoinIntPtr],[Define to integer type capturing pointer])

if test "x$ac_cv_header_stdint_h" = xyes ; then
  AC_DEFINE([COINUTILS_HAS_STDINT_H], [1], [Define to 1 if stdint.h is available for CoinUtils])
fi

if test "x$ac_cv_header_cstdint" = xyes ; then
  AC_DEFINE([COINUTILS_HAS_CSTDINT], [1], [Define to 1 if cstdint is available for CoinUtils])
fi

#############################################################################
#                           Check whether we have windows.h                 #
#############################################################################
AC_CHECK_HEADERS([windows.h])

#############################################################################
#                           Check whether we have endian.h                  #
#############################################################################
AC_CHECK_HEADERS([endian.h])

#############################################################################
#                             COIN-OR components                            #
#############################################################################

AC_COIN_CHECK_PACKAGE_BLAS(CoinUtilsLib)
AC_COIN_CHECK_PACKAGE_LAPACK(CoinUtilsLib)
AC_COIN_CHECK_PACKAGE(Glpk,   [coinglpk], [CoinUtilsLib])
AC_COIN_CHECK_PACKAGE(Sample, [coindatasample])
AC_COIN_CHECK_PACKAGE(Netlib, [coindatanetlib])

#Do we need -lm? AC_COIN_CHECK_LIBM(CoinUtilsLib)
AC_COIN_CHECK_ZLIB(CoinUtilsLib)
AC_COIN_CHECK_BZLIB(CoinUtilsLib)
AC_COIN_CHECK_GNU_READLINE(CoinUtilsLib)

# finalize libs and cflags for building CoinUtilsLib by doing pkg-config calls
AC_COIN_FINALIZE_FLAGS(CoinUtilsLib)

#############################################################################
#                  Check for doxygen                                        #
#############################################################################

AC_COIN_DOXYGEN

##############################################################################
#                   Finishing up by writing all the output                   #
##############################################################################

# Here list all the files that configure should create (except for the
# configuration header file)
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 coinutils.pc])
AC_CONFIG_FILES([doxydoc/doxygen.conf])

# Here put the location and name of the configuration header file
AC_CONFIG_HEADER([src/config.h src/config_coinutils.h])

# Here make sure we have data files in the build directory
AC_CONFIG_LINKS(test/plan.mod:test/plan.mod)

# Finally, we let configure write all the output...
AC_COIN_FINALIZE
