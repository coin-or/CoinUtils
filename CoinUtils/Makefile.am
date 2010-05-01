# Copyright (C) 2006 International Business Machines and others.
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

# Author:  Andreas Waechter           IBM    2006-04-13

AUTOMAKE_OPTIONS = foreign

########################################################################
#                          Subdirectories                              #
########################################################################

SUBDIRS = src

# We don't want to compile the test subdirectory, unless the test target is
# specified.  But we need to list it as subdirectory to make sure that it is
# included in the tarball

if ALWAYS_FALSE
  SUBDIRS += test
endif

########################################################################
#             Additional files to be included in tarball               #
########################################################################

# Here we need include all files that are not mentioned in other Makefiles

EXTRA_DIST =

########################################################################
#                           Extra Targets                              #
########################################################################

test: all
	cd test; $(MAKE) test

unitTest: test

clean-local:
	if test -r test/Makefile; then cd test; $(MAKE) clean; fi

distclean-local:
	if test -r test/Makefile; then cd test; $(MAKE) distclean; fi

install-exec-local: install-doc

uninstall-local: uninstall-doc

.PHONY: test unitTest

########################################################################
#                  Installation of the addlibs and .pc file            #
########################################################################

if COIN_HAS_PKGCONFIG
addlibsdir = $(datadir)/coin/doc/CoinUtils
addlibs_DATA = coinutils_addlibs.txt

coinutils_addlibs.txt : coinutils.pc
	PKG_CONFIG_PATH=.:@COIN_PKG_CONFIG_PATH@ $(PKG_CONFIG) --libs coinutils > coinutils_addlibs.txt
endif

pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = coinutils.pc

########################################################################
#                         Maintainer Stuff                             #
########################################################################

CLEANFILES = coinutils_addlibs.txt

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include ../BuildTools/Makemain.inc
