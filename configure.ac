# Copyright (C) 2006,2009 International Business Machines.
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author:  Andreas Waechter            IBM    2006-04-13

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_INIT([CoinUtils],[devel],[https://github.com/coin-or/CoinUtils/issues/new],[coin-or-coinutils],[https://github.com/coin-or/CoinUtils])

AC_COPYRIGHT([
Copyright 2006 International Business Machines and others.
All Rights Reserved.
This file is part of the open source package CoinUtils which is distributed
under the Eclipse Public License.])

# List one file in the package so that the configure script can test whether
# the package is actually there.

AC_CONFIG_SRCDIR(src/CoinError.cpp)

# Do some project-level initialization work (version numbers, change prefix
# default, ...)

AC_COIN_INITIALIZE

#############################################################################
#                         Standard build tool stuff                         #
#############################################################################

# Get the name of the C++ compiler and appropriate compiler options.

AC_COIN_PROG_CXX

# This is a C++ package, set the language accordingly.

AC_LANG_PUSH(C++)

# Initialize libtool

AC_COIN_PROG_LIBTOOL

# get COINUTILS_CHECKLEVEL define, used in CoinSmartPtr and CoinPackedMatrix
AC_COIN_DEBUGLEVEL

#############################################################################
#                           Header files                                    #
#############################################################################

AC_CHECK_HEADERS([windows.h])
AC_CHECK_HEADERS([endian.h])
AC_CHECK_HEADERS([execinfo.h])

############################################################################
#                 Data type and math function definitions                  #
############################################################################

AC_COINUTILS_INT64
AC_COINUTILS_INTPTR
AC_COINUTILS_BIGTYPES
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

# Collect information required to use component libraries.

AC_COIN_CHK_LAPACK(CoinUtilsLib)
AC_COIN_CHK_PKG(Glpk,[CoinUtilsLib],[coinglpk])
AC_COIN_CHK_PKG(ASL,[CoinUtilsLib],[coinasl])
AC_COIN_CHK_PKG(Sample,[],[coindatasample],[],dataonly)
AC_COIN_CHK_PKG(Netlib,[],[coindatanetlib],[],dataonly)

AC_COIN_CHK_LIBM(CoinUtilsLib)
AC_COIN_CHK_ZLIB(CoinUtilsLib)
AC_COIN_CHK_BZLIB(CoinUtilsLib)
AC_COIN_CHK_GNU_READLINE(CoinUtilsLib)

# Finalize libs and cflags for building CoinUtilsLib.
AC_COIN_FINALIZE_FLAGS(CoinUtilsLib)

AC_LANG_POP(C++)

AC_LANG_PUSH(C++)
AC_MSG_CHECKING([whether C++11 is available])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(, [
#if __cplusplus < 201103L
#error "No C++11"
#endif
  ])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE([COINUTILS_CPLUSPLUS11], [1], [Define to 1 if CoinUtils uses C++11])],
  [AC_MSG_RESULT(no)]
)
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
AC_CONFIG_HEADERS([src/config.h src/config_coinutils.h])

# Here make sure we have data files in the build directory
AC_CONFIG_LINKS(test/plan.mod:test/plan.mod)

# Finally, we let configure write all the output...
AC_COIN_FINALIZE
