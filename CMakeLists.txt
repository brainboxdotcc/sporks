cmake_minimum_required (VERSION 3.6)
project(bot)
add_executable(bot main.cpp database.cpp stringops.cpp config.cpp regex.cpp rss.cpp modules.cpp signal.cpp statusfield.cpp)

include(buildtools/cmake/FindMYSQL.cmake)
include(buildtools/cmake/FindPCRE.cmake)

find_package(OpenSSL REQUIRED)
find_package(Aegis REQUIRED)
find_package(ZLIB REQUIRED)
if ( ZLIB_FOUND )
	include_directories( ${ZLIB_INCLUDE_DIRS} )
endif( ZLIB_FOUND )

target_link_libraries(bot dl ssl crypto z mysqlclient pcre ${ZLIB_LIBRARIES})
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -g -O3 -fPIC -rdynamic -Wno-multichar")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3")

target_link_libraries(bot)

add_library(module_infobot SHARED modules/infobot/infobot.cpp modules/infobot/response.cpp modules/infobot/readline.cpp modules/infobot/status.cpp)
add_library(module_diagnostics SHARED modules/diagnostics/diagnostics.cpp)
add_library(module_help SHARED modules/help/help.cpp)
add_library(module_config SHARED modules/config/config.cpp)
add_library(module_js SHARED modules/js/js.cpp modules/js/duktape.c modules/js/timeout.c)
add_library(module_voting SHARED modules/voting/voting.cpp)

set_target_properties(module_infobot PROPERTIES PREFIX "")
set_target_properties(module_diagnostics PROPERTIES PREFIX "")
set_target_properties(module_help PROPERTIES PREFIX "")
set_target_properties(module_config PROPERTIES PREFIX "")
set_target_properties(module_js PROPERTIES PREFIX "")
set_target_properties(module_voting PROPERTIES PREFIX "")

