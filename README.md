# CoinUtils

[![A COIN-OR Project](https://coin-or.github.io/coin-or-badge.png)](https://www.coin-or.org)

[![Latest Release](https://img.shields.io/github/v/release/coin-or/CoinUtils?sort=semver)](https://github.com/coin-or/CoinUtils/releases)

_This file is auto-generated from [config.yml](.coin-or/config.yml) using the 
[generate_readme](.coin-or/generate_readme) script.
To make changes, please edit [config.yml](.coin-or/config.yml) or the generation script._

CoinUtils is an open-source collection of classes and helper functions
that are generally useful to multiple COIN-OR projects.
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


CoinUtils is written in C++ and is released as open source under the [Eclipse Public License 2.0](http://www.opensource.org/licenses/EPL-2.0).

It is distributed under the auspices of the [COIN-OR Foundation](https://www.coin-or.org)

The CoinUtils development site is https://github.com/coin-or/CoinUtils.

## CITE

[![DOI](https://zenodo.org/badge/173466792.svg)](https://zenodo.org/badge/latestdoi/173466792)

## CURRENT BUILD STATUS

[![Windows Builds](https://github.com/coin-or/CoinUtils/actions/workflows/windows-ci.yml/badge.svg?branch=master)](https://github.com/coin-or/CoinUtils/actions/workflows/windows-ci.yml?query=branch%3Amaster)

[![Linux and MacOS Builds](https://github.com/coin-or/CoinUtils/actions/workflows/linux-ci.yml/badge.svg?branch=master)](https://github.com/coin-or/CoinUtils/actions/workflows/linux-ci.yml?query=branch%3Amaster)

## DOWNLOAD

### Docker image

There is a Docker image that provides CoinUtils, as well as other projects
in the [COIN-OR Optimization
Suite](https://github.com/coin-or/COIN-OR-OptimizationSuite) [here](https://hub.docker.com/repository/docker/coinor/coin-or-optimization-suite)

### Binaries

For newer releases, binaries will be made available as assets attached to
releases in Github
[here](https://github.com/coin-or/CoinUtils/releases). Older binaries
are archived as part of Cbc
[here](https://www.coin-or.org/download/binary/Cbc).

 * *Linux*: On Debian/Ubuntu, CoinUtils is available in the package `coinor-coinutils` and can be installed with apt. On Fedora, CoinUtils is available in the package `coin-or-CoinUtils`.
 * *Windows*: The easiest way to get CoinUtils on Windows is to download an archive as described above.
 * *Mac OS X*: The easiest way to get Cbc on Mac OS X is through [Homebrew](https://brew.sh).
   * `brew tap coin-or-tools/coinor`
   * `brew install coin-or-tools/coinor/coinutils`

Due to license incompatibilities, pre-compiled binaries lack some functionality.
If binaries are not available for your platform for the latest version and you would like to request them to be built and posted, feel free to let us know on the mailing list.

### Source

Source code can be obtained either by

 * Downloading a snapshot of the source code for the latest release version of CoinUtils from the
 [releases](https://github.com/coin-or/CoinUtils/releases) page.
 * Cloning this repository from [Github](https://github.com/coin-or/CoinUtils) or 
 * Using the [coinbrew](https://github.com/coin-or/coinbrew) script to get the project and all dependencies (recommended, see below).   

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
./coinbrew fetch CoinUtils@master
./coinbrew build CoinUtils
```
For more detailed instructions on coinbrew, see https://coin-or.github.io/coinbrew.
The `coinbrew` script will fetch the additional projects specified in the Dependencies section of [config.yml](.coin-or/config.yml).

### Without `coinbrew` (Expert users)

 * Download the source code, e.g., by cloning the git repo https://github.com/coin-or/CoinUtils
 * Download and install the source code for the dependencies listed in [config.yml](.coin-or/config.yml)
 * Build the code as follows (make sure to set PKG_CONFIG_PTH to install directory for dependencies).

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
directory will be `./build/CoinUtils/master` by default. The doxygen documentation main file
is found at `<build-dir>/doxydoc/html/index.html`.

If you don't have `doxygen` installed locally, you can use also find the
documentation [here](http://coin-or.github.io/CoinUtils/Doxygen).


## Project Links

 * [COIN-OR Initiative](http://www.coin-or.org/)
 * [Discussion formum](https://github.com/coin-or/CoinUtils/discussions)
 * [Report a bug](https://github.com/coin-or/CoinUtils/issues/new)
 * [Doxygen-generated html documentation](http://coin-or.github.io/CoinUtils/Doxygen)

