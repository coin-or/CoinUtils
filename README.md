# CoinUtils

## Introduction

CoinUtils (*Coin*-or *Util*itie*s*) is an open-source collection of classes and functions that are generally useful to more than one COIN-OR project.  These utilities include:
 * Vector classes
 * Matrix classes
 * MPS file reading
 * Comparing floating point numbers with a tolerance

-------------


## Background/Download


CoinUtils is written in C++ and is released as open source code under the [Eclipse Public License (EPL)](http://www.opensource.org/licenses/eclipse-1.0).
It is available from the [COIN-OR initiative](http://www.coin-or.org/).  

You can obtain the CoinUtils source code via subversion. The following commands may be used to obtain and build CoinUtils from the source code using subversion:
 1. svn co `https://projects.coin-or.org/svn/CoinUtils/trunk` coin-CoinUtils
 1. cd coin-CoinUtils
 1. ./configure -C
 1. make
 1. make test
 1. make install

Step 1 issues the subversion command to obtain the source code. 

Step 3 runs a configure script that generates the make file.

Step 4 builds the CoinUtils library and executable program.

Step 5 builds and runs the CoinUtils unit test program.

Step 6 Installs libraries, executables and header files in directories coin-CoinUtils/lib, coin-CoinUtils/bin and coin-CoinUtils/include.

The [BuildTools](http://projects.coin-or.org/BuildTools/wiki) project has additional details on downloading, building, and installing.

----------


## Included Projects

If you download the CoinUtils package, you get [these](https://projects.coin-or.org/CoinUtils/browser/trunk/Dependencies?format=raw) additional projects.

---------


## `Doxygen` Documentation

If you have `Doxygen` available, you can build the html documentation by typing

 `make doxydoc` 

in the directory `coin-CoinUtils`.
Then open the file `coin-CoinUtils/doxydoc/html/index.html` with a browser.
Note that this creates the documentation for the `CoinUtils` package.
If you prefer to generate the documentation only for a subset 
of these projects, you can edit the file `coin-CoinUtils/doxydoc/doxygen.conf` to exclude directories 
(using the `EXCLUDE` variable, for example).

If `Doxygen` is not available, you can use also use [this link](http://www.coin-or.org/Doxygen/CoinUtils).


## Project Links

 * [COIN-OR Initiative](http://www.coin-or.org/)
 * [mailing list](http://list.coin-or.org/mailman/listinfo/coinutils)
 * [Report a bug](https://github.com/coin-or/CoinUtils/issues)
 * [Doxygen-generated html documentation](http://www.coin-or.org/Doxygen/CoinUtils) 
