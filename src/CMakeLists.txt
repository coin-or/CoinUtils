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
  CoinAdjacencyVector.cpp
  CoinAlloc.cpp
  CoinBuild.cpp
  CoinCliqueList.cpp
  CoinConflictGraph.cpp
  CoinDenseFactorization.cpp
  CoinDenseVector.cpp
  CoinDynamicConflictGraph.cpp
  CoinError.cpp
  CoinFactorization1.cpp
  CoinFactorization2.cpp
  CoinFactorization3.cpp
  CoinFactorization4.cpp
  CoinFileIO.cpp
  CoinFinite.cpp
  CoinIndexedVector.cpp
  CoinLpIO.cpp
  CoinMessage.cpp
  CoinMessageHandler.cpp
  CoinModel.cpp
  CoinModelUseful.cpp
  CoinModelUseful2.cpp
  CoinMpsIO.cpp
  CoinOslC.h
  CoinOslFactorization.cpp
  CoinOslFactorization2.cpp
  CoinOslFactorization3.cpp
  CoinPackedMatrix.cpp
  CoinPackedVector.cpp
  CoinPackedVectorBase.cpp
  CoinParam.cpp
  CoinParamUtils.cpp
  CoinPostsolveMatrix.cpp
  CoinPrePostsolveMatrix.cpp
  CoinPresolveDoubleton.cpp
  CoinPresolveDual.cpp
  CoinPresolveDupcol.cpp
  CoinPresolveEmpty.cpp
  CoinPresolveFixed.cpp
  CoinPresolveForcing.cpp
  CoinPresolveHelperFunctions.cpp
  CoinPresolveImpliedFree.cpp
  CoinPresolveIsolated.cpp
  CoinPresolveMatrix.cpp
  CoinPresolveMonitor.cpp
  CoinPresolvePsdebug.cpp
  CoinPresolveSingleton.cpp
  CoinPresolveSubst.cpp
  CoinPresolveTighten.cpp
  CoinPresolveTripleton.cpp
  CoinPresolveUseless.cpp
  CoinPresolveZeros.cpp
  CoinRational.cpp
  CoinSearchTree.cpp
  CoinShallowPackedVector.cpp
  CoinSimpFactorization.cpp
  CoinSnapshot.cpp
  CoinStructuredModel.cpp
  CoinWarmStartBasis.cpp
  CoinWarmStartDual.cpp
  CoinWarmStartPrimalDual.cpp
  CoinWarmStartVector.cpp
  bron_kerbosch.cpp
  cgraph.cpp
  clique.cpp
  clique_extender.cpp
  clique_separation.cpp
  cut.cpp
  node_heap.cpp
  str_utils.cpp
  vint_set.cpp
  )

set(_HDRS
  CoinAdjacencyVector.hpp
  CoinAlloc.hpp
  CoinBuild.hpp
  CoinCliqueList.hpp
  CoinConflictGraph.hpp
  CoinDenseFactorization.hpp
  CoinDenseVector.hpp
  CoinDistance.hpp
  CoinDynamicConflictGraph.hpp
  CoinError.hpp
  CoinFactorization.hpp
  CoinFileIO.hpp
  CoinFinite.hpp
  CoinFloatEqual.hpp
  CoinHelperFunctions.hpp
  CoinIndexedVector.hpp
  CoinLpIO.hpp
  CoinMessage.hpp
  CoinMessageHandler.hpp
  CoinModel.hpp
  CoinModelUseful.hpp
  CoinMpsIO.hpp
  CoinOslFactorization.hpp
  CoinPackedMatrix.hpp
  CoinPackedVector.hpp
  CoinPackedVectorBase.hpp
  CoinParam.hpp
  CoinPragma.hpp
  CoinPresolveDoubleton.hpp
  CoinPresolveDual.hpp
  CoinPresolveDupcol.hpp
  CoinPresolveEmpty.hpp
  CoinPresolveFixed.hpp
  CoinPresolveForcing.hpp
  CoinPresolveImpliedFree.hpp
  CoinPresolveIsolated.hpp
  CoinPresolveMatrix.hpp
  CoinPresolveMonitor.hpp
  CoinPresolvePsdebug.hpp
  CoinPresolveSingleton.hpp
  CoinPresolveSubst.hpp
  CoinPresolveTighten.hpp
  CoinPresolveTripleton.hpp
  CoinPresolveUseless.hpp
  CoinPresolveZeros.hpp
  CoinRational.hpp
  CoinSearchTree.hpp
  CoinShallowPackedVector.hpp
  CoinSignal.hpp
  CoinSimpFactorization.hpp
  CoinSmartPtr.hpp
  CoinSnapshot.hpp
  CoinSort.hpp
  CoinStaticConflictGraph.hpp
  CoinStructuredModel.hpp
  CoinTime.hpp
  CoinTypes.hpp
  CoinUtility.hpp
  CoinUtilsConfig.h
  CoinWarmStart.hpp
  CoinWarmStartBasis.hpp
  CoinWarmStartDual.hpp
  CoinWarmStartPrimalDual.hpp
  CoinWarmStartVector.hpp
  Coin_C_defines.h
  bron_kerbosch.h
  cgraph.h
  clique.h
  clique_extender.h
  clique_separation.h
  cut.h
  node_heap.h
  str_utils.h
  vint_set.h
  )

add_library(CoinUtils ${_SRCS} ${_HDRS})
target_include_directories(CoinUtils PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
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
if(ZLIB_FOUND)
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
include(GNUInstallDirs)
install(TARGETS CoinUtils
  EXPORT ${PROJECT_NAME}Targets
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/coin
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
