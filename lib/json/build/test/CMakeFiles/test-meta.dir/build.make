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
include test/CMakeFiles/test-meta.dir/depend.make

# Include the progress variables for this target.
include test/CMakeFiles/test-meta.dir/progress.make

# Include the compile flags for this target's objects.
include test/CMakeFiles/test-meta.dir/flags.make

test/CMakeFiles/test-meta.dir/src/unit-meta.cpp.o: test/CMakeFiles/test-meta.dir/flags.make
test/CMakeFiles/test-meta.dir/src/unit-meta.cpp.o: ../test/src/unit-meta.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/brain/aegis.cpp/lib/json/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/CMakeFiles/test-meta.dir/src/unit-meta.cpp.o"
	cd /home/brain/aegis.cpp/lib/json/build/test && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test-meta.dir/src/unit-meta.cpp.o -c /home/brain/aegis.cpp/lib/json/test/src/unit-meta.cpp

test/CMakeFiles/test-meta.dir/src/unit-meta.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test-meta.dir/src/unit-meta.cpp.i"
	cd /home/brain/aegis.cpp/lib/json/build/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/brain/aegis.cpp/lib/json/test/src/unit-meta.cpp > CMakeFiles/test-meta.dir/src/unit-meta.cpp.i

test/CMakeFiles/test-meta.dir/src/unit-meta.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test-meta.dir/src/unit-meta.cpp.s"
	cd /home/brain/aegis.cpp/lib/json/build/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/brain/aegis.cpp/lib/json/test/src/unit-meta.cpp -o CMakeFiles/test-meta.dir/src/unit-meta.cpp.s

# Object files for target test-meta
test__meta_OBJECTS = \
"CMakeFiles/test-meta.dir/src/unit-meta.cpp.o"

# External object files for target test-meta
test__meta_EXTERNAL_OBJECTS = \
"/home/brain/aegis.cpp/lib/json/build/test/CMakeFiles/catch_main.dir/src/unit.cpp.o"

test/test-meta: test/CMakeFiles/test-meta.dir/src/unit-meta.cpp.o
test/test-meta: test/CMakeFiles/catch_main.dir/src/unit.cpp.o
test/test-meta: test/CMakeFiles/test-meta.dir/build.make
test/test-meta: test/CMakeFiles/test-meta.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/brain/aegis.cpp/lib/json/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable test-meta"
	cd /home/brain/aegis.cpp/lib/json/build/test && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test-meta.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/CMakeFiles/test-meta.dir/build: test/test-meta

.PHONY : test/CMakeFiles/test-meta.dir/build

test/CMakeFiles/test-meta.dir/clean:
	cd /home/brain/aegis.cpp/lib/json/build/test && $(CMAKE_COMMAND) -P CMakeFiles/test-meta.dir/cmake_clean.cmake
.PHONY : test/CMakeFiles/test-meta.dir/clean

test/CMakeFiles/test-meta.dir/depend:
	cd /home/brain/aegis.cpp/lib/json/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/brain/aegis.cpp/lib/json /home/brain/aegis.cpp/lib/json/test /home/brain/aegis.cpp/lib/json/build /home/brain/aegis.cpp/lib/json/build/test /home/brain/aegis.cpp/lib/json/build/test/CMakeFiles/test-meta.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/CMakeFiles/test-meta.dir/depend

