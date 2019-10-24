cmake_minimum_required (VERSION 3.6)
project(bot)
add_executable(bot main.cpp readline.cpp database.cpp infobot.cpp response.cpp stringops.cpp config.cpp)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -std=c++11 -pthread")
link_libraries("-lpcre -lmysqlclient")

add_subdirectory(deps/sleepy-discord)
target_link_libraries(bot sleepy-discord)
