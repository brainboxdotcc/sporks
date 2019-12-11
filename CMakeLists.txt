cmake_minimum_required (VERSION 3.6)
project(bot)
add_executable(bot main.cpp readline.cpp database.cpp infobot.cpp response.cpp stringops.cpp config.cpp regex.cpp help.cpp status.cpp rss.cpp duktape.c js.cpp timeout.c modules.cpp)

include(buildtools/cmake/FindMYSQL.cmake)
include(buildtools/cmake/FindPCRE.cmake)

find_package(OpenSSL REQUIRED)
find_package(Aegis REQUIRED)
find_package(ZLIB REQUIRED)
if ( ZLIB_FOUND )
	include_directories( ${ZLIB_INCLUDE_DIRS} )
endif( ZLIB_FOUND )

target_link_libraries(bot dl ssl crypto z mysqlclient pcre ${ZLIB_LIBRARIES})
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -g -O3 -fPIC -rdynamic")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3")

target_link_libraries(bot)

add_library(module_js SHARED modules/js.cpp)
add_library(module_infobot SHARED modules/infobot.cpp)
add_library(module_diagnostics SHARED modules/diagnostics.cpp)

set_target_properties(module_js PROPERTIES PREFIX "")
set_target_properties(module_infobot PROPERTIES PREFIX "")
set_target_properties(module_diagnostics PROPERTIES PREFIX "")

