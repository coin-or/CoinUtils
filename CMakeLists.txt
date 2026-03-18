# ---------------------------------------------------------------------------
# CoinUtils/CMakeLists.txt
# ---------------------------------------------------------------------------

# Copyright (c) Microsoft Corporation
#
# All rights reserved.
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.31)

project(CoinUtils
  VERSION   2.11.6
  LANGUAGES C CXX
)

# ---------------------------------------------------------------------------
# C++ standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ---------------------------------------------------------------------------
# Installation paths (GNUInstallDirs)
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(COINUTILS_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/Coin")
set(COINUTILS_INSTALL_CMAKEDIR  "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

# ---------------------------------------------------------------------------
# Prerequisite headers
# ---------------------------------------------------------------------------
include(CheckIncludeFileCXX)
check_include_file_cxx("cmath"  HAVE_CMATH)
check_include_file_cxx("cfloat" HAVE_CFLOAT)
if(NOT HAVE_CMATH OR NOT HAVE_CFLOAT)
  message(FATAL_ERROR "Missing required headers: cmath or cfloat.")
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(COINUTILS_SOURCES
  src/CoinAdjacencyVector.cpp
  src/CoinAlloc.cpp
  src/CoinBronKerbosch.cpp
  src/CoinBuild.cpp
  src/CoinCliqueExtender.cpp
  src/CoinCliqueList.cpp
  src/CoinCliqueSet.cpp
  src/CoinConflictGraph.cpp
  src/CoinCutPool.cpp
  src/CoinDenseFactorization.cpp
  src/CoinDenseVector.cpp
  src/CoinDynamicConflictGraph.cpp
  src/CoinError.cpp
  src/CoinFactorization1.cpp
  src/CoinFactorization2.cpp
  src/CoinFactorization3.cpp
  src/CoinFactorization4.cpp
  src/CoinFileIO.cpp
  src/CoinFinite.cpp
  src/CoinIndexedVector.cpp
  src/CoinLpIO.cpp
  src/CoinMessage.cpp
  src/CoinMessageHandler.cpp
  src/CoinModel.cpp
  src/CoinModelUseful2.cpp
  src/CoinModelUseful.cpp
  src/CoinMpsIO.cpp
  src/CoinNodeHeap.cpp
  src/CoinOddWheelSeparator.cpp
  src/CoinOslFactorization2.cpp
  src/CoinOslFactorization3.cpp
  src/CoinOslFactorization.cpp
  src/CoinPackedMatrix.cpp
  src/CoinPackedVectorBase.cpp
  src/CoinPackedVector.cpp
  src/CoinParam.cpp
  src/CoinParamUtils.cpp
  src/CoinPostsolveMatrix.cpp
  src/CoinPrePostsolveMatrix.cpp
  src/CoinPresolveDoubleton.cpp
  src/CoinPresolveDual.cpp
  src/CoinPresolveDupcol.cpp
  src/CoinPresolveEmpty.cpp
  src/CoinPresolveFixed.cpp
  src/CoinPresolveForcing.cpp
  src/CoinPresolveHelperFunctions.cpp
  src/CoinPresolveImpliedFree.cpp
  src/CoinPresolveIsolated.cpp
  src/CoinPresolveMatrix.cpp
  src/CoinPresolveMonitor.cpp
  src/CoinPresolvePsdebug.cpp
  src/CoinPresolveSingleton.cpp
  src/CoinPresolveSubst.cpp
  src/CoinPresolveTighten.cpp
  src/CoinPresolveTripleton.cpp
  src/CoinPresolveUseless.cpp
  src/CoinPresolveZeros.cpp
  src/CoinRational.cpp
  src/CoinSearchTree.cpp
  src/CoinShallowPackedVector.cpp
  src/CoinShortestPath.cpp
  src/CoinSimpFactorization.cpp
  src/CoinSnapshot.cpp
  src/CoinStaticConflictGraph.cpp
  src/CoinStructuredModel.cpp
  src/CoinWarmStartBasis.cpp
  src/CoinWarmStartDual.cpp
  src/CoinWarmStartPrimalDual.cpp
  src/CoinWarmStartVector.cpp
)

set(COINUTILS_HEADERS
  src/CoinAdjacencyVector.hpp
  src/CoinAlloc.hpp
  src/CoinBronKerbosch.hpp
  src/CoinBuild.hpp
  src/CoinCliqueExtender.hpp
  src/CoinCliqueList.hpp
  src/CoinCliqueSet.hpp
  src/CoinConflictGraph.hpp
  src/CoinCutPool.hpp
  src/CoinDenseFactorization.hpp
  src/CoinDenseVector.hpp
  src/CoinDistance.hpp
  src/CoinDynamicConflictGraph.hpp
  src/CoinError.hpp
  src/CoinFactorization.hpp
  src/CoinFileIO.hpp
  src/CoinFinite.hpp
  src/CoinFloatEqual.hpp
  src/CoinHelperFunctions.hpp
  src/CoinIndexedVector.hpp
  src/CoinLpIO.hpp
  src/CoinMessageHandler.hpp
  src/CoinMessage.hpp
  src/CoinModel.hpp
  src/CoinModelUseful.hpp
  src/CoinMpsIO.hpp
  src/CoinNodeHeap.hpp
  src/CoinOddWheelSeparator.hpp
  src/CoinOslC.h
  src/CoinOslFactorization.hpp
  src/CoinPackedMatrix.hpp
  src/CoinPackedVectorBase.hpp
  src/CoinPackedVector.hpp
  src/CoinParam.hpp
  src/CoinPragma.hpp
  src/CoinPresolveDoubleton.hpp
  src/CoinPresolveDual.hpp
  src/CoinPresolveDupcol.hpp
  src/CoinPresolveEmpty.hpp
  src/CoinPresolveFixed.hpp
  src/CoinPresolveForcing.hpp
  src/CoinPresolveImpliedFree.hpp
  src/CoinPresolveIsolated.hpp
  src/CoinPresolveMatrix.hpp
  src/CoinPresolveMonitor.hpp
  src/CoinPresolvePsdebug.hpp
  src/CoinPresolveSingleton.hpp
  src/CoinPresolveSubst.hpp
  src/CoinPresolveTighten.hpp
  src/CoinPresolveTripleton.hpp
  src/CoinPresolveUseless.hpp
  src/CoinPresolveZeros.hpp
  src/CoinRational.hpp
  src/CoinSearchTree.hpp
  src/CoinShallowPackedVector.hpp
  src/CoinShortestPath.hpp
  src/CoinSignal.hpp
  src/CoinSimpFactorization.hpp
  src/CoinSmartPtr.hpp
  src/CoinSnapshot.hpp
  src/CoinSort.hpp
  src/CoinStaticConflictGraph.hpp
  src/CoinStructuredModel.hpp
  src/CoinTime.hpp
  src/CoinTypes.h
  src/CoinUtility.hpp
  src/CoinUtilsConfig.h
  src/CoinWarmStartBasis.hpp
  src/CoinWarmStartDual.hpp
  src/CoinWarmStart.hpp
  src/CoinWarmStartPrimalDual.hpp
  src/CoinWarmStartVector.hpp
  # Ces 3 headers sont intentionnellement gardés ici : CoinUtils doit avoir
  # priorité sur Osi et Clp qui installent dans le même include/Coin/
  src/configall_system.h
  src/configall_system_msc.h
  src/config_default.h
  src/config_coinutils_default.h
)

# ---------------------------------------------------------------------------
# Library
# Library type controlled by BUILD_SHARED_LIBS (standard CMake convention)
#   cmake -DBUILD_SHARED_LIBS=ON  → shared (.so / .dll)
#   cmake -DBUILD_SHARED_LIBS=OFF → static (.a / .lib)  [default]
# ---------------------------------------------------------------------------
add_library(${PROJECT_NAME} ${COINUTILS_SOURCES})

# Alias pour usage identique en sous-projet et via find_package()
add_library(Coin::CoinUtils ALIAS ${PROJECT_NAME})

# ---------------------------------------------------------------------------
# Compile options & definitions
# ---------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    HAVE_CMATH
    HAVE_CFLOAT
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

target_compile_options(${PROJECT_NAME}
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
      /bigobj /MP /wd4996 /wd4819 /wd4251 /wd4267 /wd4244 /wd4275>
    $<$<CXX_COMPILER_ID:AppleClang>:
      -Wno-inconsistent-missing-override
      -Wno-unused-command-line-argument
      -Wno-unused-result
      -Wno-exceptions>
)

# macOS deployment target (inchangé, nécessaire pour la suite COIN-OR)
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9"
    CACHE STRING "Minimum macOS deployment version.")
endif()

# PIC : obligatoire pour .so, inoffensif pour .a
set_target_properties(${PROJECT_NAME} PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  VERSION   "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}"
)

# ---------------------------------------------------------------------------
# Includes
# ---------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${COINUTILS_INSTALL_INCLUDEDIR}>
)

# ---------------------------------------------------------------------------
# Install — cible + export set
# ---------------------------------------------------------------------------
install(
  TARGETS   ${PROJECT_NAME}
  EXPORT    CoinUtilsTargets
  ARCHIVE   DESTINATION "${CMAKE_INSTALL_LIBDIR}"  COMPONENT library
  LIBRARY   DESTINATION "${CMAKE_INSTALL_LIBDIR}"  COMPONENT library
  RUNTIME   DESTINATION "${CMAKE_INSTALL_BINDIR}"  COMPONENT library
)

# ---------------------------------------------------------------------------
# Install — headers
# ---------------------------------------------------------------------------
install(
  FILES       ${COINUTILS_HEADERS}
  DESTINATION "${COINUTILS_INSTALL_INCLUDEDIR}"
  COMPONENT   headers
)

# ---------------------------------------------------------------------------
# Install — CMake config files
# ---------------------------------------------------------------------------
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CoinUtilsConfig.cmake"
  INSTALL_DESTINATION "${COINUTILS_INSTALL_CMAKEDIR}"
  PATH_VARS
    COINUTILS_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/CoinUtilsConfigVersion.cmake"
  VERSION       "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CoinUtilsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CoinUtilsConfigVersion.cmake"
  DESTINATION "${COINUTILS_INSTALL_CMAKEDIR}"
)

install(
  EXPORT      CoinUtilsTargets
  NAMESPACE   Coin::
  DESTINATION "${COINUTILS_INSTALL_CMAKEDIR}"
)
