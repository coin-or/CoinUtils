# Copyright (C) 2006,2009 International Business Machines.
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.

## $Id$

# Author:  Andreas Waechter            IBM    2006-04-13

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_INIT([CoinUtils],[trunk],[coinutils@list.coin-or.org],[coinutils],
	[https://projects.coin-or.org/CoinUtils])
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

# Get the name of the C++ compiler and appropriate compiler options.

AC_COIN_PROG_CXX

# This is a C++ package, set the language accordingly.

AC_LANG_PUSH(C++)

# Sort out what we need to do to link to Fortran libraries. Check if there's a
# Fortran compiler and call the appropriate macro based on the result. If
# there's no Fortran compiler, it's necessary to specify a library and
# function to use when attempting a link. For CoinUtils, libblas (-lblas) and
# DAXPY are appropriate choices.
# Yes, we should be able to leverage this and avoid the check for BLAS below,
# but I haven't quite figured it out yet.

AC_COIN_PROG_F77
if test "$F77" != "unavailable" ; then
  AC_COIN_F77_SETUP
else
  AC_COIN_F77_WRAPPERS(blas,DAXPY)
fi

# Initialize libtool

AC_COIN_PROG_LIBTOOL

#############################################################################
#                           Header files                                    #
#############################################################################

AC_CHECK_HEADERS([windows.h])
AC_CHECK_HEADERS([endian.h])

############################################################################
#                 Data type and math function definitions                  #
############################################################################

AC_COINUTILS_INT64
AC_COINUTILS_INTPTR
AC_COIN_CHECK_ISFINITE
AC_COIN_CHECK_ISNAN

#############################################################################
#                         Thread configuration                              #
#############################################################################

# Define new options: 
#    --enable-coinutils-threads 
#    --enable-coinutils-mempool-override-new
#    --enable-coinutils-mempool-maxpooled

# Thread-aware features are disabled by default. Specifying
# --enable-coinutils-threads on the command line will turn them on.
AC_COINUTILS_THREADS(no)

# Configure the CoinUtils mempool. Set the default maximum block size to 4096
# bytes. This can be overridden at configuration by specifying a value, as
# --enable-coinutils-mempool-maxpooled=NNN
# or at run time with environment variable
#   COINUTILS_MEMPOOL_MAXPOOLED=NNN
# Do not redirect global new/delete to the CoinUtils mempool.
# 170909  (lh) Changing the second parameter to yes causes compilation errors
#         in CoinAlloc with C++11. Likely the best solution is to remove the
#         dynamic throw spec.
AC_COINUTILS_MEMPOOL(4096,no)

#############################################################################
#                             COIN-OR components                            #
#############################################################################

AC_COIN_CHK_PKG_BLAS(CoinUtilsLib)
AC_COIN_CHK_PKG_LAPACK(CoinUtilsLib)
AC_COIN_CHK_PKG(Glpk,[CoinUtilsLib],[],[build],[])
AC_COIN_CHK_PKG(Sample,[],[coindatasample])
AC_COIN_CHK_PKG(Netlib,[],[coindatanetlib])

AC_COIN_CHECK_LIBM(CoinUtilsLib)
AC_COIN_CHECK_ZLIB(CoinUtilsLib)
AC_COIN_CHECK_BZLIB(CoinUtilsLib)
AC_COIN_CHECK_GNU_READLINE(CoinUtilsLib)

# Finalize libs and cflags for building CoinUtilsLib.
AC_COIN_FINALIZE_FLAGS(CoinUtilsLib)

AC_LANG_POP(C++)

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
