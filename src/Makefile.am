# Copyright (C) 2006 International Business Machines and others.
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.

## $Id$

# Author:  Andreas Waechter           IBM    2006-04-13

include $(top_srcdir)/BuildTools/Makemain.inc

########################################################################
#                             libCoinUtils                             #
########################################################################

# Name of the library compiled in this directory.
# We want it to be installed in the $libdir directory
lib_LTLIBRARIES = libCoinUtils.la

# List all source files for this library
libCoinUtils_la_SOURCES = \
	CoinAlloc.cpp \
	CoinBuild.cpp \
	CoinDenseVector.cpp \
	CoinError.cpp \
	CoinFactorization1.cpp \
	CoinFactorization2.cpp \
	CoinFactorization3.cpp \
	CoinFactorization4.cpp \
	CoinSimpFactorization.cpp \
	CoinDenseFactorization.cpp \
	CoinOslFactorization.cpp \
	CoinOslFactorization2.cpp \
	CoinOslFactorization3.cpp \
	CoinFileIO.cpp \
	CoinFinite.cpp \
	CoinIndexedVector.cpp \
	CoinLpIO.cpp \
	CoinMessage.cpp \
	CoinMessageHandler.cpp \
	CoinModel.cpp \
	CoinStructuredModel.cpp \
	CoinModelUseful.cpp \
	CoinModelUseful2.cpp \
	CoinMpsIO.cpp \
	CoinPackedMatrix.cpp \
	CoinPackedVector.cpp \
	CoinPackedVectorBase.cpp \
	CoinParam.cpp CoinParamUtils.cpp \
	CoinPostsolveMatrix.cpp \
	CoinPrePostsolveMatrix.cpp \
	CoinPresolveDoubleton.cpp \
	CoinPresolveDual.cpp \
	CoinPresolveDupcol.cpp \
	CoinPresolveEmpty.cpp \
	CoinPresolveFixed.cpp \
	CoinPresolveForcing.cpp \
	CoinPresolveHelperFunctions.cpp \
	CoinPresolveImpliedFree.cpp \
	CoinPresolveIsolated.cpp \
	CoinPresolveMatrix.cpp \
	CoinPresolvePsdebug.cpp \
	CoinPresolveMonitor.cpp \
	CoinPresolveSingleton.cpp \
	CoinPresolveSubst.cpp \
	CoinPresolveTighten.cpp \
	CoinPresolveTripleton.cpp \
	CoinPresolveUseless.cpp \
	CoinPresolveZeros.cpp \
	CoinRational.cpp \
	CoinSearchTree.cpp \
	CoinShallowPackedVector.cpp \
	CoinSnapshot.cpp \
	CoinWarmStartBasis.cpp \
	CoinWarmStartVector.cpp \
	CoinWarmStartDual.cpp \
	CoinWarmStartPrimalDual.cpp \
	CoinAdjacencyVector.cpp \
	CoinBronKerbosch.cpp \
	CoinCliqueExtender.cpp \
	CoinCliqueList.cpp \
	CoinCliqueSet.cpp \
	CoinConflictGraph.cpp \
	CoinCutPool.cpp \
	CoinDynamicConflictGraph.cpp \
	CoinNodeHeap.cpp \
	CoinOddWheelSeparator.cpp \
	CoinShortestPath.cpp \
	CoinStaticConflictGraph.cpp

# This is for libtool.
libCoinUtils_la_LDFLAGS = $(LT_LDFLAGS)

# Add library dependencies to libCoinUtils.
libCoinUtils_la_LIBADD = $(COINUTILSLIB_LFLAGS)

# Here list all include flags, relative to this "srcdir" directory.
AM_CPPFLAGS = $(COINUTILSLIB_CFLAGS)

########################################################################
#                Headers that need to be installed                     #
########################################################################

# Here list all the header files that are required by a user of the library,
# and that therefore should be installed in pkgincludedir.
includecoindir = $(pkgincludedir)
includecoin_HEADERS = \
	Coin_C_defines.h \
	CoinAlloc.hpp \
	CoinBuild.hpp \
	CoinDenseVector.hpp \
	CoinDistance.hpp \
	CoinError.hpp \
	CoinFactorization.hpp \
	CoinSimpFactorization.hpp \
	CoinDenseFactorization.hpp \
	CoinOslFactorization.hpp \
	CoinFileIO.hpp \
	CoinFinite.hpp \
	CoinFloatEqual.hpp \
	CoinHelperFunctions.hpp \
	CoinIndexedVector.hpp \
	CoinLpIO.hpp \
	CoinMessage.hpp \
	CoinMessageHandler.hpp \
	CoinModel.hpp \
	CoinStructuredModel.hpp \
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
	CoinPresolveMonitor.hpp \
	CoinPresolvePsdebug.hpp \
	CoinPresolveSingleton.hpp \
	CoinPresolveSubst.hpp \
	CoinPresolveTighten.hpp \
	CoinPresolveTripleton.hpp \
	CoinPresolveUseless.hpp \
	CoinPresolveZeros.hpp \
	CoinRational.hpp \
	CoinSearchTree.hpp \
	CoinShallowPackedVector.hpp \
	CoinSignal.hpp \
	CoinSmartPtr.hpp \
	CoinSnapshot.hpp \
	CoinSort.hpp \
	CoinTime.hpp \
	CoinTypes.h \
	CoinUtility.hpp \
	CoinWarmStart.hpp \
	CoinWarmStartBasis.hpp \
	CoinWarmStartVector.hpp \
	CoinWarmStartDual.hpp \
	CoinWarmStartPrimalDual.hpp \
	CoinAdjacencyVector.hpp \
	CoinBronKerbosch.hpp \
	CoinCliqueExtender.hpp \
	CoinCliqueList.hpp \
	CoinCliqueSet.hpp \
	CoinConflictGraph.hpp \
	CoinCutPool.hpp \
	CoinDynamicConflictGraph.hpp \
	CoinNodeHeap.hpp \
	CoinOddWheelSeparator.hpp \
	CoinShortestPath.hpp \
	CoinStaticConflictGraph.hpp

#######################################################################
# Create the Config.h file that has all public defines and install it #
#######################################################################

install-exec-local:
	$(install_sh_DATA) config_coinutils.h $(DESTDIR)$(includecoindir)/CoinUtilsConfig.h

uninstall-local:
	rm -f $(DESTDIR)$(includecoindir)/CoinUtilsConfig.h
