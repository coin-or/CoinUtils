# Select optimization (-O or -g). -O will be automatically bumped up to the 
# highest level of optimization the compiler supports. If want something in
# between then specify the exact level you want, e.g., -O1 or -O2
OptLevel := -g

##############################################################################
# You should not need to edit below this line.
##############################################################################
# The location of the customized Makefiles
export CoinDir = $(shell cd ../..; pwd)
export MakefileDir := $(CoinDir)/Makefiles
include ${MakefileDir}/Makefile.coin
include ${MakefileDir}/Makefile.location

IncDir  := ${CoinIncDir} ${zlibIncDir} ${bzlibIncDir} $(lapackIncDir)
LibDir  := ${CoinLibDir} ${zlibLibDir} ${bzlibLibDir} $(lapackLibDir)
LibName := ${CoinLibName} ${zlibLibName} ${bzlibLibName} $(lapackLibName)
Define  := ${CoinDefine} ${zlibDefine} ${bzlibDefine} $(lapackDefine)

TESTSRC :=

TESTSRC += CoinDenseVectorTest.cpp
TESTSRC += CoinIndexedVectorTest.cpp
TESTSRC += CoinMpsIOTest.cpp
TESTSRC += CoinPackedMatrixTest.cpp
TESTSRC += CoinPackedVectorTest.cpp
TESTSRC += CoinShallowPackedVectorTest.cpp
TESTSRC += CoinErrorTest.cpp
TESTSRC += unitTest.cpp

##############################################################################
# You should not need to edit below this line.
##############################################################################

CXXFLAGS += $(OPTFLAG)
ifeq ($(OptLevel),-g)
    CXXFLAGS += -DCOIN_DEBUG
endif

###############################################################################

space:= $(empty) $(empty)
OptVersion := $(subst $(space),_,$(OptLevel))

TARGETDIR := $(UNAME)$(OptVersion)
DEPDIR := dep

VPATH := . : include : Junk : ${TARGETDIR} : ${DEPDIR}

#########################################################################

CXXFLAGS += $(addprefix -I,${IncDir})
CXXFLAGS += $(addprefix -D,${Define})

LIBDIRS := ${LibDir}
LIBS    := ${LibName}

LDFLAGS := $(addprefix -L,$(LIBDIRS))
LDFLAGS += $(call ifprefix,$(SHLINKPREFIX),$(LIBDIRS))
LDFLAGS += $(patsubst lib%,-l%,$(basename $(LIBS)))

###############################################################################

TESTOBJ := $(addprefix $(TARGETDIR)/, $(TESTSRC:.cpp=.o))
TESTDEP := $(addprefix $(DEPDIR)/, $(TESTSRC:.cpp=.d))

###############################################################################
# This one must be right before the rules

include ${MakefileDir}/Makefile.rules

###############################################################################

.DELETE_ON_ERROR:

.PHONY: default clean doc unitTest libdepend

default: unitTest

###############################################################################

unitTest: $(TARGETDIR)/unitTest

libdepend :
	(cd .. && ${MAKE} install)

$(TARGETDIR)/unitTest : libdepend $(TESTOBJ)
	@rm -rf Junk
	@echo ""
	@echo Creating unitTest
	@mkdir -p $(TARGETDIR)
	@rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ $(TESTOBJ) $(LDFLAGS) $(SYSLD) -lm
	${CP} $@ ..

###############################################################################

doc:
	doxygen $(MakefileDir)/doxygen.conf

clean :
	@rm -rf Junk
	@rm -rf $(UNAME)
	@rm -rf dep

###############################################################################

%::
	@mkdir -p Junk
	touch Junk/$(notdir $@)

###############################################################################

-include $(TESTDEP)
