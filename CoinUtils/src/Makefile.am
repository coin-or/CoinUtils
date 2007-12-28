# Copyright (C) 2006 International Business Machines and others.
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

# Author:  Andreas Waechter           IBM    2006-04-13

AUTOMAKE_OPTIONS = foreign

########################################################################
#                             libCoinUtils                             #
########################################################################

# Name of the library compiled in this directory.  We want it to be installed
# in the 'lib' directory
lib_LTLIBRARIES = libCoinUtils.la

# AW: This is a fix at the moment to head with the fact that LIBS has
# -llapack and other libraries in it when those are used and not compiled
# here.  The content of LIBS is passed to libtool when building a shared
# library, and that can cause trouble, since liblapack.a and others
# should not be built into the shared libraries.
LIBS =

# List all source files for this library, including headers
libCoinUtils_la_SOURCES = \
	CoinUtilsConfig.h \
	Coin_C_defines.h \
	CoinBuild.cpp CoinBuild.hpp \
	CoinDenseVector.cpp CoinDenseVector.hpp \
	CoinDistance.hpp \
	CoinError.cpp CoinError.hpp \
	CoinFactorization.hpp \
	CoinFactorization1.cpp \
	CoinFactorization2.cpp \
	CoinFactorization3.cpp \
	CoinFactorization4.cpp \
	CoinFileIO.cpp CoinFileIO.hpp \
	CoinFinite.hpp \
	CoinFloatEqual.hpp \
	CoinHelperFunctions.hpp \
	CoinIndexedVector.cpp CoinIndexedVector.hpp \
	CoinLpIO.cpp CoinLpIO.hpp \
	CoinMessage.cpp CoinMessage.hpp \
	CoinMessageHandler.cpp CoinMessageHandler.hpp \
	CoinModel.cpp CoinModel.hpp \
	CoinModelUseful.cpp CoinModelUseful.hpp \
	CoinModelUseful2.cpp \
	CoinMpsIO.cpp CoinMpsIO.hpp \
	CoinPackedMatrix.cpp CoinPackedMatrix.hpp \
	CoinPackedVector.cpp CoinPackedVector.hpp \
	CoinPackedVectorBase.cpp CoinPackedVectorBase.hpp \
	CoinParam.cpp CoinParamUtils.cpp CoinParam.hpp \
	CoinPostsolveMatrix.cpp \
	CoinPragma.hpp \
	CoinPrePostsolveMatrix.cpp \
	CoinPresolveDoubleton.cpp CoinPresolveDoubleton.hpp \
	CoinPresolveDual.cpp CoinPresolveDual.hpp \
	CoinPresolveDupcol.cpp CoinPresolveDupcol.hpp \
	CoinPresolveEmpty.cpp CoinPresolveEmpty.hpp \
	CoinPresolveFixed.cpp CoinPresolveFixed.hpp \
	CoinPresolveForcing.cpp CoinPresolveForcing.hpp \
	CoinPresolveHelperFunctions.cpp \
	CoinPresolveImpliedFree.cpp CoinPresolveImpliedFree.hpp \
	CoinPresolveIsolated.cpp CoinPresolveIsolated.hpp \
	CoinPresolveMatrix.cpp CoinPresolveMatrix.hpp \
	CoinPresolvePsdebug.cpp CoinPresolvePsdebug.hpp \
	CoinPresolveSingleton.cpp CoinPresolveSingleton.hpp \
	CoinPresolveSubst.cpp CoinPresolveSubst.hpp \
	CoinPresolveTighten.cpp CoinPresolveTighten.hpp \
	CoinPresolveTripleton.cpp CoinPresolveTripleton.hpp \
	CoinPresolveUseless.cpp CoinPresolveUseless.hpp \
	CoinPresolveZeros.cpp CoinPresolveZeros.hpp \
	CoinSearchTree.cpp CoinSearchTree.hpp \
	CoinShallowPackedVector.cpp CoinShallowPackedVector.hpp \
	CoinSignal.hpp \
	CoinSmartPtr.hpp \
	CoinSnapshot.cpp CoinSnapshot.hpp \
	CoinSort.hpp \
	CoinTime.hpp \
	CoinTypes.hpp \
	CoinUtility.hpp \
	CoinWarmStart.hpp \
	CoinWarmStartBasis.cpp CoinWarmStartBasis.hpp \
	CoinWarmStartDual.cpp CoinWarmStartDual.hpp

# This is for libtool (on Windows)
libCoinUtils_la_LDFLAGS = $(LT_LDFLAGS)

# Here we list additional libraries that we want to include, such as Lapack
# and Blas when they are compiled with the rest of COIN.  This is temporary
# solution.
libCoinUtils_la_LIBADD = $(COINUTILS_LIBADD)

libCoinUtils_la_DEPENDENCIES = $(COINUTILS_LIBADD)

# Here list all include flags, relative to this "srcdir" directory.  This
# "cygpath" stuff is necessary to compile with native compilers on Windows
AM_CPPFLAGS =
if COIN_HAS_GMPL
  AM_CPPFLAGS += -I`$(CYGPATH_W) $(GMPLINCDIR)`
endif

# This line is necessary to allow VPATH compilation with MS compilers
# on Cygwin
DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

########################################################################
#                Headers that need to be installed                     #
########################################################################

# Here list all the header files that are required by a user of the library,
# and that therefore should be installed in 'install/coin'
includecoindir = $(includedir)/coin
includecoin_HEADERS = \
	../inc/config_coinutils.h \
	Coin_C_defines.h \
	CoinBuild.hpp \
	CoinDenseVector.hpp \
	CoinDistance.hpp \
	CoinError.hpp \
	CoinFactorization.hpp \
	CoinFileIO.hpp \
	CoinFinite.hpp \
	CoinFloatEqual.hpp \
	CoinHelperFunctions.hpp \
	CoinIndexedVector.hpp \
	CoinLpIO.hpp \
	CoinMessage.hpp \
	CoinMessageHandler.hpp \
	CoinModel.hpp \
	CoinModelUseful.hpp \
	CoinMpsIO.hpp \
	CoinPackedMatrix.hpp \
	CoinPackedVector.hpp \
	CoinPackedVectorBase.hpp \
	CoinParam.hpp \
	CoinPragma.hpp \
	CoinPresolveDoubleton.hpp \
	CoinPresolveDual.hpp \
	CoinPresolveDupcol.hpp \
	CoinPresolveEmpty.hpp \
	CoinPresolveFixed.hpp \
	CoinPresolveForcing.hpp \
	CoinPresolveImpliedFree.hpp \
	CoinPresolveIsolated.hpp \
	CoinPresolveMatrix.hpp \
	CoinPresolvePsdebug.hpp \
	CoinPresolveSingleton.hpp \
	CoinPresolveSubst.hpp \
	CoinPresolveTighten.hpp \
	CoinPresolveTripleton.hpp \
	CoinPresolveUseless.hpp \
	CoinPresolveZeros.hpp \
	CoinSearchTree.hpp \
	CoinShallowPackedVector.hpp \
	CoinSignal.hpp \
	CoinSmartPtr.hpp \
	CoinSnapshot.hpp \
	CoinSort.hpp \
	CoinTime.hpp \
	CoinTypes.hpp \
	CoinUtility.hpp \
	CoinWarmStart.hpp \
	CoinWarmStartBasis.hpp \
	CoinWarmStartDual.hpp

#############################################################################
# Create the Config.h file that always defines HAVE_CONFIG_H and install it #
#############################################################################

# You only need to adapt the following line
ConfigHeader = CoinUtilsConfig.h

install-exec-local:
	echo "#ifndef HAVE_CONFIG_H" >bla
	echo "#define HAVE_CONFIG_H" >>bla
	echo "#endif" >> bla
	cat $(srcdir)/$(ConfigHeader) >> bla
	$(install_sh_DATA) bla $(DESTDIR)$(includecoindir)/$(ConfigHeader)
	rm -f bla

uninstall-local:
	rm -f $(DESTDIR)$(includecoindir)/$(ConfigHeader)
