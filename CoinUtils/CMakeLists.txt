set(NAME "COINUTILS")

# PTHREAD
if(PTHREADS_FOUND)
  set(${NAME}_PTHREADS "1" CACHE INTERNAL "Use pthread")
endif()

set(COIN_${NAME}_CHECKLEVEL "0" CACHE INTERNAL
  "${NAME} check level")
set(COIN_${NAME}_VERBOSITY "0" CACHE INTERNAL
  "${NAME} verbosity level")
configure_file(config.h.cmake.in config.h)
configure_file(config_coinutils.h.cmake.in config_coinutils.h)

set(_SRCS
  src/CoinAlloc.cpp
  src/CoinBuild.cpp
  src/CoinDenseVector.cpp
  src/CoinError.cpp
  src/CoinFactorization1.cpp
  src/CoinFactorization2.cpp
  src/CoinFactorization3.cpp
  src/CoinFactorization4.cpp
  src/CoinSimpFactorization.cpp
  src/CoinDenseFactorization.cpp
  src/CoinOslFactorization.cpp
  src/CoinOslFactorization2.cpp
  src/CoinOslFactorization3.cpp
  src/CoinFileIO.cpp
  src/CoinFinite.cpp
  src/CoinIndexedVector.cpp
  src/CoinLpIO.cpp
  src/CoinMessage.cpp
  src/CoinMessageHandler.cpp
  src/CoinModel.cpp
  src/CoinStructuredModel.cpp
  src/CoinModelUseful.cpp
  src/CoinModelUseful2.cpp
  src/CoinMpsIO.cpp
  src/CoinPackedMatrix.cpp
  src/CoinPackedVector.cpp
  src/CoinPackedVectorBase.cpp
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
  src/CoinPresolvePsdebug.cpp
  src/CoinPresolveMonitor.cpp
  src/CoinPresolveSingleton.cpp
  src/CoinPresolveSubst.cpp
  src/CoinPresolveTighten.cpp
  src/CoinPresolveTripleton.cpp
  src/CoinPresolveUseless.cpp
  src/CoinPresolveZeros.cpp
  src/CoinSearchTree.cpp
  src/CoinShallowPackedVector.cpp
  src/CoinSnapshot.cpp
  src/CoinWarmStartBasis.cpp
  src/CoinWarmStartVector.cpp
  src/CoinWarmStartDual.cpp
  src/CoinWarmStartPrimalDual.cpp
  src/CoinRational.cpp)

set(_HDRS
  src/Coin_C_defines.h
  src/CoinUtilsConfig.h
  src/CoinAlloc.hpp
  src/CoinBuild.hpp
  src/CoinDenseVector.hpp
  src/CoinDistance.hpp
  src/CoinError.hpp
  src/CoinFactorization.hpp
  src/CoinSimpFactorization.hpp
  src/CoinDenseFactorization.hpp
  src/CoinOslFactorization.hpp
  src/CoinFileIO.hpp
  src/CoinFinite.hpp
  src/CoinFloatEqual.hpp
  src/CoinHelperFunctions.hpp
  src/CoinIndexedVector.hpp
  src/CoinLpIO.hpp
  src/CoinMessage.hpp
  src/CoinMessageHandler.hpp
  src/CoinModel.hpp
  src/CoinStructuredModel.hpp
  src/CoinModelUseful.hpp
  src/CoinMpsIO.hpp
  src/CoinPackedMatrix.hpp
  src/CoinPackedVector.hpp
  src/CoinPackedVectorBase.hpp
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
  src/CoinSearchTree.hpp
  src/CoinShallowPackedVector.hpp
  src/CoinSignal.hpp
  src/CoinSmartPtr.hpp
  src/CoinSnapshot.hpp
  src/CoinSort.hpp
  src/CoinTime.hpp
  src/CoinTypes.hpp
  src/CoinUtility.hpp
  src/CoinWarmStart.hpp
  src/CoinWarmStartBasis.hpp
  src/CoinWarmStartVector.hpp
  src/CoinWarmStartDual.hpp
  src/CoinWarmStartPrimalDual.hpp
  src/CoinRational.hpp)

include(GNUInstallDirs)

add_library(CoinUtils ${_SRCS} ${_HDRS})
target_include_directories(CoinUtils PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include/coin>)
target_compile_definitions(CoinUtils
  PUBLIC HAVE_CONFIG_H
  PRIVATE COINUTILS_BUILD)
if(CMAKE_VERSION VERSION_LESS "3.8.2")
  set_property(TARGET CoinUtils PROPERTY CXX_STANDARD 11)
  set_property(TARGET CoinUtils PROPERTY CXX_STANDARD_REQUIRED ON)
else()
  target_compile_features(CoinUtils PUBLIC cxx_std_11)
endif()
if(APPLE)
  set_target_properties(CoinUtils PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
  set_target_properties(CoinUtils PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
if(TARGET ZLIB::ZLIB)
  target_link_libraries(CoinUtils PRIVATE	ZLIB::ZLIB)
endif()
if(CMAKE_USE_PTHREADS_INIT)
  target_link_libraries(CoinUtils PRIVATE	Threads::Threads)
endif()
set_target_properties(CoinUtils PROPERTIES
  PUBLIC_HEADER "${_HDRS};${CMAKE_CURRENT_BINARY_DIR}/config_coinutils.h"
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR})
add_library(Coin::CoinUtils ALIAS CoinUtils)

# Install
install(TARGETS CoinUtils
  EXPORT ${PROJECT_NAME}Targets
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/coin
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
