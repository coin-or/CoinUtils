# Copyright (C) 2006 International Business Machines and others.
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

# Author:  Andreas Waechter           IBM    2006-04-13

AUTOMAKE_OPTIONS = foreign

########################################################################
#                             libCoinBase                              #
########################################################################

# Name of the library compiled in this directory.  We want it to be installed
# in the 'lib' directory
lib_LTLIBRARIES = libCoinBase.la

# List all source files for this library, including headers
libCoinBase_la_SOURCES = \
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
	CoinShallowPackedVector.cpp CoinShallowPackedVector.hpp \
	CoinSignal.hpp \
	CoinSort.hpp \
	CoinTime.hpp \
	CoinTypes.hpp \
	CoinUtility.hpp \
	CoinWarmStart.hpp \
	CoinWarmStartBasis.cpp CoinWarmStartBasis.hpp \
	CoinWarmStartDual.cpp CoinWarmStartDual.hpp

# This is for libtool (on Windows)
libCoinBase_la_LDFLAGS = -no-undefined

# Here list all include flags, relative to this "srcdir" directory.  This
# "cygpath" stuff is necessary to compile with native compilers on Windows
CXXFLAGS +=

########################################################################
#                Headers that need to be installed                     #
########################################################################

# Here list all the header files that are required by a user of the library,
# and that therefore should be installed in 'install'
include_HEADERS = \
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
	CoinShallowPackedVector.hpp \
	CoinSignal.hpp \
	CoinSort.hpp \
	CoinTime.hpp \
	CoinTypes.hpp \
	CoinUtility.hpp \
	CoinWarmStart.hpp \
	CoinWarmStartBasis.hpp \
	CoinWarmStartDual.hpp

########################################################################
#     Generating the configure header that is to be distributed        #
########################################################################

# You should only need to adapt the following three lines
CONFIG_DIST = CoinBaseConfig.hpp
CONFIG_H = config_coinbase.h
$(libCoinBase_la_OBJECTS): ../inc/$(CONFIG_DIST)

nodist_include_HEADERS = ../inc/$(CONFIG_DIST)

../inc/$(CONFIG_DIST): ../inc/$(CONFIG_H)
	echo "/** Required defines from $(CONFIG_H) */" >bla.tmp
	echo "#ifndef __$(CONFIG_DIST)__" | sed -e 's/\./_/g' >>bla.tmp
	echo "#define __$(CONFIG_DIST)__" | sed -e 's/\./_/g' >>bla.tmp
	$(EGREP) 'COIN_HAS' $< >>bla.tmp
	echo "#endif" >>bla.tmp
	if test -r ../inc/$(CONFIG_DIST); then \
	  if diff ./inc/$(CONFIG_DIST) bla.tmp >/dev/null 2>1 ; then \
	    rm -f bla.tmp; \
	  else \
	    rm -f ../inc/$(CONFIG_DIST); \
	    mv bla.tmp ../inc/$(CONFIG_DIST); \
	  fi; \
	else \
	  mv bla.tmp ../inc/$(CONFIG_DIST); \
	fi

CLEANFILES = bla.tmp

DISTCLEANFILES = $(CONFIG_DIST)
