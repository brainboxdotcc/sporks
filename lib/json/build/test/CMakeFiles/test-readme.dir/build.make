# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/brain/aegis.cpp/lib/json

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/brain/aegis.cpp/lib/json/build

# Include any dependencies generated for this target.
include test/CMakeFiles/test-readme.dir/depend.make

# Include the progress variables for this target.
include test/CMakeFiles/test-readme.dir/progress.make

# Include the compile flags for this target's objects.
include test/CMakeFiles/test-readme.dir/flags.make

test/CMakeFiles/test-readme.dir/src/unit-readme.cpp.o: test/CMakeFiles/test-readme.dir/flags.make
test/CMakeFiles/test-readme.dir/src/unit-readme.cpp.o: ../test/src/unit-readme.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/brain/aegis.cpp/lib/json/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/CMakeFiles/test-readme.dir/src/unit-readme.cpp.o"
	cd /home/brain/aegis.cpp/lib/json/build/test && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test-readme.dir/src/unit-readme.cpp.o -c /home/brain/aegis.cpp/lib/json/test/src/unit-readme.cpp

test/CMakeFiles/test-readme.dir/src/unit-readme.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test-readme.dir/src/unit-readme.cpp.i"
	cd /home/brain/aegis.cpp/lib/json/build/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/brain/aegis.cpp/lib/json/test/src/unit-readme.cpp > CMakeFiles/test-readme.dir/src/unit-readme.cpp.i

test/CMakeFiles/test-readme.dir/src/unit-readme.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test-readme.dir/src/unit-readme.cpp.s"
	cd /home/brain/aegis.cpp/lib/json/build/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/brain/aegis.cpp/lib/json/test/src/unit-readme.cpp -o CMakeFiles/test-readme.dir/src/unit-readme.cpp.s

# Object files for target test-readme
test__readme_OBJECTS = \
"CMakeFiles/test-readme.dir/src/unit-readme.cpp.o"

# External object files for target test-readme
test__readme_EXTERNAL_OBJECTS = \
"/home/brain/aegis.cpp/lib/json/build/test/CMakeFiles/catch_main.dir/src/unit.cpp.o"

test/test-readme: test/CMakeFiles/test-readme.dir/src/unit-readme.cpp.o
test/test-readme: test/CMakeFiles/catch_main.dir/src/unit.cpp.o
test/test-readme: test/CMakeFiles/test-readme.dir/build.make
test/test-readme: test/CMakeFiles/test-readme.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/brain/aegis.cpp/lib/json/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable test-readme"
	cd /home/brain/aegis.cpp/lib/json/build/test && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test-readme.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/CMakeFiles/test-readme.dir/build: test/test-readme

.PHONY : test/CMakeFiles/test-readme.dir/build

test/CMakeFiles/test-readme.dir/clean:
	cd /home/brain/aegis.cpp/lib/json/build/test && $(CMAKE_COMMAND) -P CMakeFiles/test-readme.dir/cmake_clean.cmake
.PHONY : test/CMakeFiles/test-readme.dir/clean

test/CMakeFiles/test-readme.dir/depend:
	cd /home/brain/aegis.cpp/lib/json/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/brain/aegis.cpp/lib/json /home/brain/aegis.cpp/lib/json/test /home/brain/aegis.cpp/lib/json/build /home/brain/aegis.cpp/lib/json/build/test /home/brain/aegis.cpp/lib/json/build/test/CMakeFiles/test-readme.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/CMakeFiles/test-readme.dir/depend

