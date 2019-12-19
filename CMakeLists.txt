cmake_minimum_required (VERSION 3.6)
project(bot)
add_executable(bot main.cpp database.cpp stringops.cpp config.cpp regex.cpp modules.cpp signal.cpp statusfield.cpp)

string(ASCII 27 Esc)

include(buildtools/cmake/FindMYSQL.cmake)
include(buildtools/cmake/FindPCRE.cmake)

find_package(OpenSSL REQUIRED)
find_package(Aegis REQUIRED)
find_package(ZLIB REQUIRED)
if ( ZLIB_FOUND )
	include_directories( ${ZLIB_INCLUDE_DIRS} )
endif( ZLIB_FOUND )

include_directories( "include" )

target_link_libraries(bot dl ssl crypto z mysqlclient pcre ${ZLIB_LIBRARIES})
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -g -O3 -fPIC -rdynamic -Wno-multichar")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3")

target_link_libraries(bot)

set (modules_dir "modules")
file(GLOB subdirlist ${modules_dir}/*)
foreach (fullmodname ${subdirlist})
	get_filename_component(modname ${fullmodname} NAME)
	message(STATUS "Found module '${Esc}[1;34mmodule_${modname}${Esc}[m'")
	set (modsrc "")
	aux_source_directory(${modules_dir}/${modname} modsrc)
	add_library(module_${modname} SHARED ${modsrc})
	set_target_properties(module_${modname} PROPERTIES PREFIX "")
endforeach(fullmodname)

