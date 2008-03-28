# Copyright (C) 2006 International Business Machines.
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

# Author:  Andreas Waechter            IBM    2006-04-13

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([CoinUtils],[2.2.1],[http://projects.coin-or.org/CoinUtils])

AC_COPYRIGHT([
Copyright 2006 International Business Machines and others.
All Rights Reserved.
This file is part of the open source package Coin which is distributed
under the Common Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(src/CoinError.cpp)

# Where should everything be installed by default?  Here, we want it
# to be installed directly in 'bin', 'lib', 'include' subdirectories
# of the directory where configure is run.  The default would be
# /usr/local.      
AC_PREFIX_DEFAULT([`pwd`])

#############################################################################
#                         Standard build tool stuff                         #
#############################################################################

# Get the system type
AC_CANONICAL_BUILD

# If this project depends on external projects, the Externals file in
# the source root directory contains definition of where to find those
# externals.  The following macro ensures that those externals are
# retrieved by svn if they are not there yet.
AC_COIN_PROJECTDIR_INIT

# Check if user wants to produce debugging code
AC_COIN_DEBUG_COMPILE(CoinUtils)

# Get the name of the C++ compiler and appropriate compiler options
AC_COIN_PROG_CXX

# Check if we have a Fortran compiler; if we do, acquire the appropriate
# options. This has to be done before we call AC_COIN_INIT_AUTO_TOOLS, else
# it will helpfully set FFLAGS to an incorrect value.
AC_COIN_FIND_F77
if test "$F77" != unavailable ; then
  AC_COIN_PROG_F77
  AC_COIN_F77_WRAPPERS
fi

# Initialize automake and libtool
AC_COIN_INIT_AUTO_TOOLS

############################################################################
#                   Stuff that we need for finite and isnan                #
############################################################################

AC_LANG_PUSH(C++)

################
# Header Files #
################
AC_COIN_CHECK_CXX_CHEADER(math)
AC_COIN_CHECK_CXX_CHEADER(float)
AC_COIN_CHECK_CXX_CHEADER(ieeefp)
AC_COIN_CHECK_CXX_CHEADER(assert)

###################
# Function finite #
###################
MY_C_FINITE=
AC_CHECK_DECL([finite],[MY_C_FINITE=finite],,[
#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# endif
#endif
#ifdef HAVE_CFLOAT
# include <cfloat>
#else
# ifdef HAVE_FLOAT_H
#  include <float.h>
# endif
#endif
#ifdef HAVE_CIEEEFP
# include <cieeefp>
#else
# ifdef HAVE_IEEEFP_H
#  include <ieeefp.h>
# endif
#endif])
if test -z "$MY_C_FINITE"; then
  AC_CHECK_DECL([_finite],[MY_C_FINITE=_finite],,[
#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# endif
#endif
#ifdef HAVE_CFLOAT
# include <cfloat>
#else
# ifdef HAVE_FLOAT_H
#  include <float.h>
# endif
#endif
#ifdef HAVE_CIEEEFP
# include <cieeefp>
#else
# ifdef HAVE_IEEEFP_H
#  include <ieeefp.h>
# endif
#endif])
  if test -z "$MY_C_FINITE"; then
    AC_CHECK_DECL([isfinite],[MY_C_FINITE=isfinite],,[
#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# endif
#endif
#ifdef HAVE_CFLOAT
# include <cfloat>
#else
# ifdef HAVE_FLOAT_H
#  include <float.h>
# endif
#endif
#ifdef HAVE_CIEEEFP
# include <cieeefp>
#else
# ifdef HAVE_IEEEFP_H
#  include <ieeefp.h>
# endif
#endif])
  fi
fi
if test -z "$MY_C_FINITE"; then
  AC_MSG_WARN(Cannot find C-function for checking Inf. Check will test whether the value is different from DBL_MAX)
else
  AC_DEFINE_UNQUOTED([MY_C_FINITE],[$MY_C_FINITE],
                     [Define to be the name of C-function for Inf check])
fi

###################
# Function isnan #
###################
MY_C_ISNAN=
AC_CHECK_DECL([isnan],[MY_C_ISNAN=isnan],,[
#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# endif
#endif
#ifdef HAVE_CFLOAT
# include <cfloat>
#else
# ifdef HAVE_FLOAT_H
#  include <float.h>
# endif
#endif
#ifdef HAVE_CIEEEFP
# include <cieeefp>
#else
# ifdef HAVE_IEEEFP_H
#  include <ieeefp.h>
# endif
#endif])

# It appears that for some systems (e.g., Mac OSX), cmath will provide only
# std::isnan, and bare isnan will be unavailable. Typically we need a parameter
# in the test to allow C++ to do overload resolution.

if test -z "$MY_C_ISNAN"; then
  AC_CHECK_DECL([std::isnan(42.42)],[MY_C_ISNAN=std::isnan],,[
#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# endif
#endif
#ifdef HAVE_CFLOAT
# include <cfloat>
#else
# ifdef HAVE_FLOAT_H
#  include <float.h>
# endif
#endif
#ifdef HAVE_CIEEEFP
# include <cieeefp>
#else
# ifdef HAVE_IEEEFP_H
#  include <ieeefp.h>
# endif
#endif])
fi

if test -z "$MY_C_ISNAN"; then
  AC_CHECK_DECL([_isnan],[MY_C_ISNAN=_isnan],,[
#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# endif
#endif
#ifdef HAVE_CFLOAT
# include <cfloat>
#else
# ifdef HAVE_FLOAT_H
#  include <float.h>
# endif
#endif
#ifdef HAVE_CIEEEFP
# include <cieeefp>
#else
# ifdef HAVE_IEEEFP_H
#  include <ieeefp.h>
# endif
#endif])
fi
if test -z "$MY_C_ISNAN"; then
  AC_CHECK_DECL([isnand],[MY_C_ISNAN=isnand],,[
#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# endif
#endif
#ifdef HAVE_CFLOAT
# include <cfloat>
#else
# ifdef HAVE_FLOAT_H
#  include <float.h>
# endif
#endif
#ifdef HAVE_CIEEEFP
# include <cieeefp>
#else
# ifdef HAVE_IEEEFP_H
#  include <ieeefp.h>
# endif
#endif])
fi
if test -z "$MY_C_ISNAN"; then
  AC_MSG_WARN(Cannot find C-function for checking NaN. Check will return false)
else
  AC_DEFINE_UNQUOTED([MY_C_ISNAN],[$MY_C_ISNAN],
                     [Define to be the name of C-function for NaN check])
fi

#############################################################################
#                       Thread configuration				    #
#############################################################################

# Define new options: 
#    --enable-coinutils-threads 
#    --enable-coinutils-mempool-override-new
#    --enable-coinutils-mempool-maxpooled

AC_ARG_ENABLE([coinutils-threads],
[AC_HELP_STRING([--enable-coinutils-threads],
                [enables compilation of thread aware CoinUtils (mempool so far)])])

if test "$enable_coinutils_threads" = yes; then
  # Define the preprocessor macro
  AC_DEFINE([COINUTILS_PTHREADS],[1],[Define to 1 if the thread aware version of CoinUtils should be compiled])
  AC_CHECK_LIB([rt],[clock_gettime],
               [ADDLIBS="-lrt $ADDLIBS"],
	       [AC_MSG_ERROR([--enable-coinutils-threads selected, but -lrt unavailable])])
  AC_CHECK_LIB([pthread],[pthread_create],
               [ADDLIBS="-lpthread $ADDLIBS"],
	       [AC_MSG_ERROR([--enable-coinutils-threads selected, but -lpthreads unavailable])])
fi

AC_ARG_ENABLE([coinutils-mempool-override-new],
[AC_HELP_STRING([--enable-coinutils-mempool-override-new],
                [enables the CoinUtils mempool to override global new/delete])])

if test "$enable_coinutils_mempool_override_new" = yes; then
   AC_DEFINE([COINUTILS_MEMPOOL_OVERRIDE_NEW],[1],[Define to 1 CoinUtils should override global new/delete])
fi

AC_ARG_ENABLE([coinutils-mempool-maxpooled],
[AC_HELP_STRING([--enable-coinutils-mempool-maxpooled],
                [Specify the default maximum memory allocation size that is served by the memory pool. If negative (or 'no') then the memory pool is disabled completely. Otherwise its value can be overridden at runtime using the COINUTILS_MEMPOOL_MAXPOOLED environment variable.])])

if test "$enable_coinutils_mempool_maxpooled" = yes; then
   AC_DEFINE([COINUTILS_MEMPOOL_MAXPOOLED],[4096],[Default maximum pooled allocation size])
elif test "$enable_coinutils_mempool_maxpooled" = no; then
   AC_DEFINE([COINUTILS_MEMPOOL_MAXPOOLED],[-1],[Disable CoinUtils memory pool])
elif test x"$enable_coinutils_mempool_maxpooled" = x; then
   AC_DEFINE([COINUTILS_MEMPOOL_MAXPOOLED],[-1],[Disable CoinUtils memory pool])
else
   AC_DEFINE_UNQUOTED([COINUTILS_MEMPOOL_MAXPOOLED],${enable_coinutils_mempool_maxpooled},[Default maximum pooled allocation size])
fi

#############################################################################
#                       Finding certain integer types                       #
#############################################################################

AC_COIN_CHECK_CXX_CHEADER(inttypes)

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
AC_DEFINE_UNQUOTED([COIN_INT64_T],[$CoinInt64],[Define to 64bit integer type])
AC_DEFINE_UNQUOTED([COIN_UINT64_T],[$CoinInt64],
		   [Define to 64bit unsigned integer type])


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

#############################################################################
#                              COIN components                              #
#############################################################################

AC_COIN_HAS_PROJECT(CoinUtils)

#############################################################################
#                             Other dependencies                            #
#############################################################################

# For Gnu Mathprog (GMPL), supplied by glpk.

AC_COIN_HAS_GLPK(Gmpl)
if test x"$use_thirdpartyglpk" = xbuild; then
  COINUTILS_LIBADD="../../ThirdParty/Glpk/libcoinglpk.la $COINUTILS_LIBADD"
fi

#############################################################################
#                       Other third party software                          #
#############################################################################

# Ampl Solver library
AC_COIN_HAS_ASL

# Check for BLAS and LAPACK. If there's no Fortran compiler available, skip
# this entirely, otherwise configuration will fail when we try to do a Fortran
# link. The check cannot be embedded in AC_COIN_HAS_BLAS because the offending
# macro is AC_REQUIRE'd from somewhere inside AC_TRY_LINK and ends up out in
# front of all the code in AC_COIN_HAS_BLAS.  This will need to be revisited if
# we run up against a situation where a prebuilt BLAS/LAPACK is linkable but
# there's no Fortran compiler. This code has the unfortunate property that
# calls to AM_CONDITIONAL are exposed here and still occur in AC_COIN_HAS_BLAS
# and AC_COIN_HAS_LAPACK. With some thought, it should be possible to separate
# BLAS and LAPACK configuration into two steps that would maintain information
# hiding and still avoid attempting a Fortran link when no compiler is
# present.

if test "$F77" = unavailable ; then
  coin_has_blas=no
  AM_CONDITIONAL([COIN_HAS_BLAS],[false])
  AM_CONDITIONAL([COIN_BUILD_BLAS],[false])
  coin_has_lapack=no
  AM_CONDITIONAL([COIN_HAS_LAPACK],[false])
  AM_CONDITIONAL([COIN_BUILD_LAPACK],[false])
else

  #######################
  #        BLAS         #
  #######################

  AC_COIN_HAS_BLAS
  if test $coin_has_blas = no; then
    AC_MSG_WARN([Cannot find BLAS - no dense factorization])
  fi
  if test "$use_blas" = BUILD; then
    COINUTILS_LIBADD="../../ThirdParty/Blas/libcoinblas.la $COINUTILS_LIBADD"
  fi

  #######################
  #       LAPACK        #
  #######################

  AC_COIN_HAS_LAPACK
  if test $coin_has_lapack = no; then
    AC_MSG_WARN([Cannot find LAPACK - no dense factorization])
  fi
  if test "$use_lapack" = BUILD; then
    COINUTILS_LIBADD="../../ThirdParty/Lapack/libcoinlapack.la $COINUTILS_LIBADD"
  fi
fi

# For now, we just add the lapack and blas libraries here in case they are
# compiled in ThirdParty.  On the long run, we need a different solution
AC_SUBST(COINUTILS_LIBADD)

AC_COIN_CHECK_GNU_ZLIB
AC_COIN_CHECK_GNU_BZLIB

##############################################################################
#                   Finishing up by writing all the output                   #
##############################################################################

# Here list all the files that configure should create (except for the
# configuration header file)
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
		 coinutils_addlibs.txt])

# Here put the location and name of the configuration header file
AC_CONFIG_HEADER([inc/config_coinutils.h])

# Finally, we let configure write all the output...
AC_COIN_FINALIZE
