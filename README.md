# CoinUtils 2.11

[![A COIN-OR Project](https://coin-or.github.io/coin-or-badge.png)](https://www.coin-or.org)

[![Latest Release](https://img.shields.io/github/v/release/coin-or/CoinUtils?sort=semver)](https://github.com/coin-or/CoinUtils/releases)

_This file is auto-generated from [config.yml](.coin-or/config.yml) using the 
[generate_readme](https://github.com/coin-or/coinbrew/tree/master/scripts/generate_readme).
To make changes, please edit [config.yml](.coin-or/config.yml) or the generation script._

CoinUtils (*Coin*-OR *Util*itie*s*) is an open-source collection of classes and functions that are generally useful to more than one COIN-OR project.
These utilities include:
 * classes for storing and manipulating sparse matrices and vectors,
 * performing matrix factorization,
 * parsing input files in standard formats, e.g. MPS,
 * building representations of mathematical programs,
 * performing simple presolve operations,
 * warm starting algorithms for mathematical programs,
 * comparing floating point numbers with a tolerance
 * classes for storing and manipulating conflict graphs, and
 * classes for searching and storing cliques and odd cycles in conflict graphs, among others.

CoinUtils is written in C++ and is released as open source under the [Eclipse Public License 2.0](http://www.opensource.org/licenses/eclipse-2.0).

It is distributed under the auspices of the [COIN-OR Foundation](https://www.coin-or.org)

The CoinUtils website is https://github.com/coin-or/CoinUtils.

## CITE

[![DOI](https://zenodo.org/badge/173466792.svg)](https://zenodo.org/badge/latestdoi/173466792)

## CURRENT BUILD STATUS

[![Build Status](https://travis-ci.org/coin-or/CoinUtils.svg?branch=stable/2.11)](https://travis-ci.org/coin-or/CoinUtils)

[![Build status](https://ci.appveyor.com/api/projects/status/a41muofrtpdw18c5/branch/stable/2.11?svg=true)](https://ci.appveyor.com/project/tkralphs/coinutils-6jtnc/branch/stable/2.11)

## DOWNLOAD

Binaries for most platforms are available as part of [Cbc](https://bintray.com/coin-or/download/Cbc). 

 * *Linux*: On Debian/Ubuntu, CoinUtils is available in the package `coinor-coinutils` and can be installed with apt. On Fedora, CoinUtils is available in the package `coin-or-CoinUtils`.
 * *Windows*: The easiest way to get CoinUtils on Windows is to download from *[Bintray](https://bintray.com/coin-or/download/Cbc)*.
 * *Mac OS X*: The easiest way to get Cbc on Mac OS X is through [Homebrew](https://brew.sh).
   * `brew tap coin-or-tools/coinor`
   * `brew install coinutils`

Due to license incompatibilities, pre-compiled binaries lack some functionality.
If binaries are not available for your platform for the latest version and you would like to request them to be built and posted, feel free to let us know on the mailing list.

*Source code* can be obtained either by

 * Downloading a snapshot of the source code for the latest release version of CoinUtils from the
 [releases](https://github.com/coin-or/CoinUtils/releases) page.
 * Cloning the repository from [Github](https://github.com/coin-or/CoinUtils) or using the 
`coinbrew` script (recommended).  

Below is a quick start guide for building on common platforms. More detailed
build instructions are
[here](https://coin-or.github.io/user_introduction.html).

## BUILDING from source

The quick start assumes you are in a bash shell. 

### Using `coinbrew`

To build CoinUtils from source, obtain the `coinbrew` script, do
```
wget https://raw.githubusercontent.com/coin-or/coinbrew/master/coinbrew
chmod u+x coinbrew
./coinbrew fetch CoinUtils@stable/2.11
./coinbrew build CoinUtils
```
For more detailed instructions on coinbrew, see https://coin-or.github.io/coinbrew.
The `coinbrew` script will fetch the additional projects specified in the Dependencies section of [config.yml](.coin-or/config.yml).

### Without `coinbrew` (Expert users)

Obtain the source code, e.g., by cloning the git repo https://github.com/coin-or/CoinUtils
```
./configure -C
make
make test
make install
```

## Doxygen Documentation

If you have `Doxygen` available, you can build a HTML documentation by typing

`make doxygen-docs` 

in the build directory. If CoinUtils was built via `coinbrew`, then the build
directory will be `./build/CoinUtils/version` by default. The doxygen documentation main file
is found at `<build-dir>/doxydoc/html/index.html`.

If you don't have `doxygen` installed locally, you can use also find the
documentation [here](http://coin-or.github.io/CoinUtils/Doxygen).

## Project Links

 * [COIN-OR Initiative](http://www.coin-or.org/)
 * [Mailing list](http://list.coin-or.org/mailman/listinfo/coinutils)
 * [Report a bug](https://github.com/coin-or/CoinUtils/issues/new)
 * [Doxygen-generated html documentation](http://coin-or.github.io/CoinUtils/Doxygen)

