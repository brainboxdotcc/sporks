cmake_minimum_required (VERSION 3.6)
project(bot)
add_executable(bot main.cpp readline.cpp database.cpp infobot.cpp response.cpp stringops.cpp config.cpp regex.cpp help.cpp status.cpp rss.cpp duktape.c js.cpp timeout.c)

include(buildtools/cmake/FindMYSQL.cmake)
include(buildtools/cmake/FindPCRE.cmake)

find_package(OpenSSL REQUIRED)
find_package(Aegis REQUIRED)
find_package(ZLIB REQUIRED)
if ( ZLIB_FOUND )
	include_directories( ${ZLIB_INCLUDE_DIRS} )
endif( ZLIB_FOUND )

target_link_libraries(bot ssl crypto z mysqlclient pcre ${ZLIB_LIBRARIES})
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -g -O3")

target_link_libraries(bot)
