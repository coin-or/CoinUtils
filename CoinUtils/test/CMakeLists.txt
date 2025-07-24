add_executable(CoinUtilsTest)
target_sources(CoinUtilsTest PRIVATE
  CoinLpIOTest.cpp
	CoinDenseVectorTest.cpp
	CoinErrorTest.cpp
	CoinIndexedVectorTest.cpp
	CoinMessageHandlerTest.cpp
	CoinModelTest.cpp
	CoinMpsIOTest.cpp
	CoinPackedMatrixTest.cpp
	CoinPackedVectorTest.cpp
	CoinShallowPackedVectorTest.cpp
	unitTest.cpp)
target_include_directories(CoinUtilsTest PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/coin>)
target_compile_definitions(CoinUtilsTest
  PUBLIC HAVE_CONFIG_H
  PRIVATE COINUTILS_BUILD)
target_compile_features(CoinUtilsTest PUBLIC cxx_std_11)
if(APPLE)
  set_target_properties(CoinUtilsTest PROPERTIES
    INSTALL_RPATH "@loader_path;@loader_path/../${CMAKE_INSTALL_LIBDIR}")
elseif(UNIX)
  set_target_properties(CoinUtilsTest PROPERTIES
    INSTALL_RPATH "$ORIGIN:$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()
target_link_libraries(CoinUtilsTest PRIVATE Coin::CoinUtils)
set_target_properties(CoinUtilsTest PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR})
add_executable(Coin::CoinUtilsTest ALIAS CoinUtilsTest)
add_test(NAME CoinUtilsTest COMMAND CoinUtilsTest)
