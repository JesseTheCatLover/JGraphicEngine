# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "D:\Tools\CLion 2024.1.4\bin\cmake\win\x64\bin\cmake.exe"

# The command to remove a file.
RM = "D:\Tools\CLion 2024.1.4\bin\cmake\win\x64\bin\cmake.exe" -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "E:\C++ Projects\JGraphicEngine\JGraphicEngine"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "E:\C++ Projects\JGraphicEngine\JGraphicEngine\cmake-build-debug"

# Include any dependencies generated for this target.
include CMakeFiles/STB_IMAGE.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/STB_IMAGE.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/STB_IMAGE.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/STB_IMAGE.dir/flags.make

CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.obj: CMakeFiles/STB_IMAGE.dir/flags.make
CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.obj: CMakeFiles/STB_IMAGE.dir/includes_CXX.rsp
CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.obj: E:/C++\ Projects/JGraphicEngine/JGraphicEngine/Source/ThirdParty/stb_image.cpp
CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.obj: CMakeFiles/STB_IMAGE.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir="E:\C++ Projects\JGraphicEngine\JGraphicEngine\cmake-build-debug\CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.obj"
	D:\Tools\MinGW\bin\c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.obj -MF CMakeFiles\STB_IMAGE.dir\Source\ThirdParty\stb_image.cpp.obj.d -o CMakeFiles\STB_IMAGE.dir\Source\ThirdParty\stb_image.cpp.obj -c "E:\C++ Projects\JGraphicEngine\JGraphicEngine\Source\ThirdParty\stb_image.cpp"

CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.i"
	D:\Tools\MinGW\bin\c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "E:\C++ Projects\JGraphicEngine\JGraphicEngine\Source\ThirdParty\stb_image.cpp" > CMakeFiles\STB_IMAGE.dir\Source\ThirdParty\stb_image.cpp.i

CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.s"
	D:\Tools\MinGW\bin\c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "E:\C++ Projects\JGraphicEngine\JGraphicEngine\Source\ThirdParty\stb_image.cpp" -o CMakeFiles\STB_IMAGE.dir\Source\ThirdParty\stb_image.cpp.s

# Object files for target STB_IMAGE
STB_IMAGE_OBJECTS = \
"CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.obj"

# External object files for target STB_IMAGE
STB_IMAGE_EXTERNAL_OBJECTS =

libSTB_IMAGE.a: CMakeFiles/STB_IMAGE.dir/Source/ThirdParty/stb_image.cpp.obj
libSTB_IMAGE.a: CMakeFiles/STB_IMAGE.dir/build.make
libSTB_IMAGE.a: CMakeFiles/STB_IMAGE.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir="E:\C++ Projects\JGraphicEngine\JGraphicEngine\cmake-build-debug\CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libSTB_IMAGE.a"
	$(CMAKE_COMMAND) -P CMakeFiles\STB_IMAGE.dir\cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\STB_IMAGE.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/STB_IMAGE.dir/build: libSTB_IMAGE.a
.PHONY : CMakeFiles/STB_IMAGE.dir/build

CMakeFiles/STB_IMAGE.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\STB_IMAGE.dir\cmake_clean.cmake
.PHONY : CMakeFiles/STB_IMAGE.dir/clean

CMakeFiles/STB_IMAGE.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" "E:\C++ Projects\JGraphicEngine\JGraphicEngine" "E:\C++ Projects\JGraphicEngine\JGraphicEngine" "E:\C++ Projects\JGraphicEngine\JGraphicEngine\cmake-build-debug" "E:\C++ Projects\JGraphicEngine\JGraphicEngine\cmake-build-debug" "E:\C++ Projects\JGraphicEngine\JGraphicEngine\cmake-build-debug\CMakeFiles\STB_IMAGE.dir\DependInfo.cmake" "--color=$(COLOR)"
.PHONY : CMakeFiles/STB_IMAGE.dir/depend

