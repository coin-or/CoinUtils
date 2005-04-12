##############################################################################
# The location of the customized Makefiles
export CoinDir = $(shell cd ..; pwd)
export MakefileDir := $(CoinDir)/Makefiles

##############################################################################
# Static or shared libraries should be built (STATIC or SHARED)?
LibType := SHARED
#LibType := STATIC

# Select optimization (-O or -g). -O will be automatically bumped up to the 
# highest level of optimization the compiler supports. If want something in
# between then specify the exact level you want, e.g., -O1 or -O2
OptLevel := -g
OptLevel := -O1

# Look at the ${CoinDir}/Makefiles/Makefile.location file, comment in which
# libraries are/will be available and edit the location of the various
# libraries

##############################################################################
# You should not need to edit below this line.
###############################################################################

LIBNAME := Coin

LIBSRC =

LIBSRC += CoinFactorization1.cpp
LIBSRC += CoinFactorization2.cpp
LIBSRC += CoinFactorization3.cpp
LIBSRC += CoinFactorization4.cpp
LIBSRC += CoinIndexedVector.cpp
LIBSRC += CoinMessage.cpp
LIBSRC += CoinError.cpp
LIBSRC += CoinMessageHandler.cpp
LIBSRC += CoinMpsIO.cpp
LIBSRC += CoinFileIO.cpp
LIBSRC += CoinPackedMatrix.cpp
LIBSRC += CoinPackedVector.cpp
LIBSRC += CoinPackedVectorBase.cpp
LIBSRC += CoinShallowPackedVector.cpp
LIBSRC += CoinDenseVector.cpp
LIBSRC += CoinWarmStartBasis.cpp
LIBSRC += CoinWarmStartDual.cpp
LIBSRC += CoinBuild.cpp
LIBSRC += CoinModel.cpp
LIBSRC += CoinModelUseful.cpp
LIBSRC += CoinModelUseful2.cpp
LIBSRC += CoinPostsolveMatrix.cpp
LIBSRC += CoinPrePostsolveMatrix.cpp
LIBSRC += CoinPresolveMatrix.cpp
LIBSRC += CoinPresolveDoubleton.cpp
LIBSRC += CoinPresolveEmpty.cpp
LIBSRC += CoinPresolveFixed.cpp
LIBSRC += CoinPresolvePsdebug.cpp
LIBSRC += CoinPresolveSingleton.cpp
LIBSRC += CoinPresolveZeros.cpp
LIBSRC += CoinPresolveDual.cpp		    
LIBSRC += CoinPresolveDupcol.cpp		    
LIBSRC += CoinPresolveForcing.cpp		    
LIBSRC += CoinPresolveHelperFunctions.cpp	    
LIBSRC += CoinPresolveImpliedFree.cpp	    
LIBSRC += CoinPresolveIsolated.cpp	    
LIBSRC += CoinPresolveSubst.cpp		    
LIBSRC += CoinPresolveTighten.cpp		    
LIBSRC += CoinPresolveTripleton.cpp		    
LIBSRC += CoinPresolveUseless.cpp         

##############################################################################
include ${MakefileDir}/Makefile.coin
include ${MakefileDir}/Makefile.location
ifneq ($(filter COIN_libGlpk,$(CoinLibsDefined)),)
# We will allow for use of GMPL
# Note - you should use a dynamic glpk library not a static one
# so you may need to modify Makefile.location to make .so
CXXFLAGS += -DCOIN_USE_GMPL
endif
ifeq ($(OptLevel),-O2)
#     CXXFLAGS += -DNDEBUG
endif

# ZEROFAULT attempts to initialize memory so that runtime memory read/write
# checks will not stumble over random uninitialised bytes due to gaps in data
# structures, lazy initialisation, etc. In order for this to be effective for
# inline functions, you need to enable this in CoinSort.hpp too.
# DEBUG_PRESOLVE, PRESOLVE_SUMMARY, and PRESOLVE_CONSISTENCY relate to
# CoinPresolve. See comments in CoinPresolvePsdebug.[cpp,hpp].

ifeq ($(OptLevel),-g)
# CXXFLAGS += -DZEROFAULT
# CXXFLAGS += -DCOIN_DEBUG
# CXXFLAGS += -DPRINT_DEBUG
# CXXFLAGS += -DPRESOLVE_DEBUG=1 -DPRESOLVE_SUMMARY=1
# CXXFLAGS += -DPRESOLVE_CONSISTENCY=1
endif

export ExtraIncDir  := ${zlibIncDir}  ${bzlibIncDir} $(lapackIncDir) \
		       $(SbbIncDir) $(OsiIncDir) $(GlpkIncDir)
export ExtraLibName := ${zlibLibName} ${bzlibLibName} $(lapackLibName) $(GlpkLibName)
export ExtraDefine  := ${zlibDefine}  ${bzlibDefine} $(lapackDefine)
export ExtraLibDir  := ${zlibLibDir}  ${bzlibLibDir} $(lapackLibDir) $(GlpkLibDir)

export LibType OptLevel LIBNAME LIBSRC

###############################################################################

.DELETE_ON_ERROR:

.PHONY: default libcoin libCoin install unitTest library clean doc

default: install

unitTest: install
	(cd Test && $(MAKE) unitTest)

install library:
	$(MAKE) -f ${MakefileDir}/Makefile.lib $@

libcoin libCoin: library

doc:
	doxygen $(MakefileDir)/doxygen.conf
###############################################################################
clean:
	@rm -rf Junk
	@rm -rf $(UNAME)*
	@rm -rf dep
	@rm -rf Test/Junk
	@rm -rf Test/$(UNAME)*
	@rm -rf Test/dep
	@rm -f unitTest
