##############################################################################
# The location of the customized Makefiles
export CoinDir = $(shell cd ..; pwd)
export MakefileDir := $(CoinDir)/Makefiles

##############################################################################
# Static or shared libraries should be built (STATIC or SHARED)?
LibType := SHARED

# Select optimization (-O or -g). -O will be automatically bumped up to the 
# highest level of optimization the compiler supports. If want something in
# between then specify the exact level you want, e.g., -O1 or -O2
OptLevel := -g
OptLevel := -O2

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
LIBSRC += CoinMessageHandler.cpp
LIBSRC += CoinMpsIO.cpp
LIBSRC += CoinPackedMatrix.cpp
LIBSRC += CoinPackedVector.cpp
LIBSRC += CoinPackedVectorBase.cpp
LIBSRC += CoinShallowPackedVector.cpp
LIBSRC += CoinWarmStartBasis.cpp

##############################################################################
include ${MakefileDir}/Makefile.coin
include ${MakefileDir}/Makefile.location

export ExtraIncDir  := ${zlibIncDir}  ${bzlibIncDir}
export ExtraLibDir  := ${zlibLibDir}  ${bzlibLibDir}
export ExtraLibName := ${zlibLibName} ${bzlibLibName}
export ExtraDefine  := ${zlibDefine}  ${bzlibDefine}

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
