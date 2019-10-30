cmake_minimum_required (VERSION 3.6)
project(bot)
add_executable(bot main.cpp readline.cpp database.cpp infobot.cpp response.cpp stringops.cpp config.cpp regex.cpp help.cpp status.cpp)

set (AEGIS_DYN_LINK 1)

find_package(Aegis REQUIRED)

link_libraries("-lpcre -lmysqlclient -lrapidjson")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -std=c++17 -pthread")

target_link_libraries(bot)
