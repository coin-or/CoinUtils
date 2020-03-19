# Copyright (C) 2006 International Business Machines and others.
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.

## $Id$

# Author:  Andreas Waechter           IBM    2006-04-13

include BuildTools/Makemain.inc

########################################################################
#                          Subdirectories                              #
########################################################################

SUBDIRS = src

# We don't want to compile the test subdirectory, unless the test target is
# specified. But we need to list it as a subdirectory to make sure that it
# is included in the tarball

if ALWAYS_FALSE
  SUBDIRS += test
endif

########################################################################
#                           Extra Targets                              #
########################################################################

test: all
	cd test; $(MAKE) test

unitTest: test

clean-local: clean-doxygen-docs
	if test -r test/Makefile; then cd test; $(MAKE) clean; fi

distclean-local:
	if test -r test/Makefile; then cd test; $(MAKE) distclean; fi

install-data-local: install-doc

uninstall-local: uninstall-doc uninstall-doxygen-docs

.PHONY: test unitTest

########################################################################
#                  Installation of the .pc file                        #
########################################################################

pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = coinutils.pc
