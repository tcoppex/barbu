# -----------------------------------------------------------------------------
# CMake configuration file (2021 - unlicense.org)
# 
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.12.0)

# -----------------------------------------------------------------------------
# CMake policies.
# -----------------------------------------------------------------------------

# Force OpenGL_GL_PREFERENCE to be set to GLVND instead of LEGACY.
if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()

# Use <PackageName>_ROOT variables.
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Do not place MSVC warning flags in the default cache entries.
if(POLICY CMP0092)
  cmake_policy(SET CMP0092 NEW)
endif()

# -----------------------------------------------------------------------------
# Project.
# -----------------------------------------------------------------------------

project(barbu VERSION 0.2 LANGUAGES CXX C)

# -----------------------------------------------------------------------------
# Options.
# -----------------------------------------------------------------------------

# GLEW is optional and not provided, by default extensions are loaded manually.
option(OPT_USE_GLEW                 "Use GLEW for OpenGL extension ?"       OFF)

# Enable clang static code analyzer.
option(OPT_ENABLE_CLANG_TIDY        "Use clang-tidy ?"                      OFF)

# Concurrency is used to speedup skinning animations and some loops.
option(OPT_USE_OPENMP               "Use OpenMP for pragma concurrency ?"   OFF)

# Enable RTTI.
option(OPT_ENABLE_RTTI              "Use RTTI ?"                            OFF)

# Verbose logs are disabled by default.
option(OPT_ENABLE_DEBUG_LOG         "Enable complementary debug log ?"      OFF)

# Choose how to compile the libraries.
option(OPT_BUILD_SHARED_LIBS        "Compile libraries as shared ?"         ON)

if (OPT_BUILD_SHARED_LIBS)
  set(BARBU_LIB_TYPE SHARED)
else()
  set(BARBU_LIB_TYPE STATIC)
endif()


# -----------------------------------------------------------------------------
# Project paths.
# -----------------------------------------------------------------------------

set(BARBU_ROOT_PATH                 ${CMAKE_CURRENT_SOURCE_DIR})

# Existing directories.
set(BARBU_SOURCE_DIR                ${BARBU_ROOT_PATH}/src)
set(BARBU_SHADERS_DIR               ${BARBU_SOURCE_DIR}/shaders)
set(BARBU_ASSETS_DIR                ${BARBU_ROOT_PATH}/assets)
set(BARBU_THIRD_PARTY_DIR           ${BARBU_ROOT_PATH}/third_party)
set(BARBU_TOOLS_DIR                 ${BARBU_ROOT_PATH}/tools)

# Created (output) directories.
set(BARBU_BINARY_DIR                ${BARBU_ROOT_PATH}/bin)

# [ Dangerous ] Mark the binary directory for removal using 'clean'.
#set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES ${BARBU_BINARY_DIR})

# -----------------------------------------------------------------------------
# CMake includes.
# -----------------------------------------------------------------------------

# CMake module path.
list(APPEND CMAKE_MODULE_PATH       ${BARBU_TOOLS_DIR}/cmake)

# Custom CMake helpers functions.
include(Helpers)

# -----------------------------------------------------------------------------
# Determine the host's total number of processors.
# -----------------------------------------------------------------------------

include(ProcessorCount)
ProcessorCount(BARBU_NPROC_MAX)

# -----------------------------------------------------------------------------
# CMake settings.
# -----------------------------------------------------------------------------

set(CMAKE_VERBOSE_MAKEFILE          OFF)
set(CMAKE_COLOR_MAKEFILE            ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD   ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS   ON)
set(CMAKE_DISABLE_SOURCE_CHANGES    OFF)

# Default standards with no extensions.
set(CMAKE_C_STANDARD                11)
set(CMAKE_CXX_STANDARD              17)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_EXTENSIONS            OFF)

# Remove (and disallow) sneaky default flags for targets.
set(CMAKE_CXX_FLAGS                 "" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_DEBUG           "" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_MINSIZEREL      "" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_RELEASE         "" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "" CACHE INTERNAL "")

# Specify a postfix on debug targets.
#set(CMAKE_DEBUG_POSTFIX             d)

# Force a specific output directory. Used to bypass MSVC settings.
helpers_setGlobalOutputDirectory(${BARBU_BINARY_DIR})

# Force debug mode by default when no target is specified.
# (when set with generator working at cache generation time like Makefile / Ninja).
helpers_setDefaultBuildType("Debug")

# -----------------------------------------------------------------------------
# Internal configuration attributes.
# -----------------------------------------------------------------------------

# Determines if the build is flagged for full debug mode.
if(CMAKE_BUILD_TYPE MATCHES "Debug")
  set(DEBUG TRUE)
  message(STATUS "DEBUG build.")
else()
  set(DEBUG FALSE)
endif()

# Determines the build platform.
if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT WEBGL)
  set(OS_NAME "Linux")
  set(OS_LINUX TRUE)
elseif(WIN32)
  set(OS_NAME "Windows")
  set(OS_WIN32 TRUE)
elseif(ANDROID)
  set(OS_NAME "Android")
  set(OS_ANDROID TRUE)

  message(WARNING "\"${CMAKE_SYSTEM_NAME}\" support is a Work In Progress.")
else()
  message(WARNING "The \"${CMAKE_SYSTEM_NAME}\" platform is not officially supported.")
endif()

# Determines the current compiler.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(USE_GCC TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "(Apple)?[Cc]lang")
  set(USE_CLANG TRUE)
elseif(MSVC)
  set(USE_MSVC TRUE)
else()
  message(WARNING "The \"${CMAKE_CXX_COMPILER_ID}\" compiler is not officially supported.")
endif()

# -----------------------------------------------------------------------------
# Setup code analyzer.
# -----------------------------------------------------------------------------

# Setup clang-tidy parameters (if enabled and found).
if(OPT_ENABLE_CLANG_TIDY)  
  # Note : options must be in the same line to work [fixme].
  helpers_setClangTidy(
    #--checks=
    clang-analyzer-*,google-*,-google-build-using-namespace#,cppcoreguidelines-*,-cppcoreguidelines-avoid-magic-numbers
    #--header-filter=
    ${BARBU_SOURCE_DIR}
  )
endif()

# -----------------------------------------------------------------------------
# Compiler settings.
# -----------------------------------------------------------------------------

# Check the compiler version.
if(USE_GCC)
  # https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2017
  helpers_checkCompilerVersion(${CMAKE_CXX_COMPILER_ID} 7.1)
elseif(USE_CLANG)
  helpers_checkCompilerVersion(${CMAKE_CXX_COMPILER_ID} 11.0)
elseif(USE_MSVC)
  helpers_checkCompilerVersion(${CMAKE_CXX_COMPILER_ID} 14.0) # 15.7 is ideal
endif()

# Specify custom compiler and linker flags.
if(USE_GCC OR USE_CLANG)
  # This is a mix of warning options not available in all GCC / Clang version.
  list(APPEND CXX_FLAGS 
    -Wall
    -Wextra -Wpedantic
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wdouble-promotion
    -Wuseless-cast
    -Wcast-align
    -Wlogical-op
    -Wnull-dereference
    # -Wshadow
    -Wmisleading-indentation
    -Wformat=2 -Wformat-truncation -Wformat-overflow
    -Wduplicated-cond
    -Wno-unknown-pragmas 
    -Wno-unused-function 
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    # -Wzero-as-null-pointer-constant
    -Wdisabled-optimization
    -Wvector-operation-performance
    # -Wold-style-cast
    # -Wduplicated-branches 
    # -Wconversion
  )

  # To avoid warnings about the previous mismatched GCC / Clang options.
  if (USE_CLANG)
    list(APPEND CXX_FLAGS         -Wno-unknown-warning-option)
  endif()

  # Features.
  list(APPEND CXX_FLAGS 
    -fno-strict-aliasing 
    -fno-builtin-memcmp
    -fno-common
    -ffunction-sections
    -fdata-sections
    # -fno-exceptions
    # -ftree-loop-distribution
    # -funroll-loops
  )
  
  # Force ANSI colored outputs.
  if(USE_GCC)
    add_compile_options(-fdiagnostics-color=always)
  elseif (USE_CLANG)
    add_compile_options(-fcolor-diagnostics)
  endif()

  if(NOT OPT_ENABLE_RTTI)
    list(APPEND CXX_FLAGS -fno-rtti)
  endif()

  list(APPEND CXX_FLAGS_DEBUG           -O0 -g3 -Wundef) #-Wconversion)
  list(APPEND CXX_FLAGS_RELEASE         -O2 -msse4.1)
  list(APPEND CXX_FLAGS_RELWITHDEBINFO  ${CXX_FLAGS_RELEASE} -g1)
  
  # Linker flags.
  list(APPEND CustomLinkFlags 
    -Wl,--gc-sections 
  )

  # [improve] Used only for static library compilation.
  if(NOT OPT_BUILD_SHARED_LIBS)
    list(APPEND CXX_FLAGS         -fvisibility=hidden)
    list(APPEND CustomLinkFlags   -Wl,--no-undefined)
  endif()

  # Definitions.
  if(NOT DEBUG)
    list(APPEND CustomDefinitions 
      -DNDEBUG=1
    )
  endif()

  # For C++17 filesystem support, might became depecrated.
  set(STD_CXX_FILESYSTEM_LIB stdc++fs)

elseif(USE_MSVC)
  
  list(APPEND CXX_FLAGS
    /W4 
    /w14640   # enable warning on thread un-safe static member initialization.
    /w14928   # illegal copy-initialization.
    /w14265   # class has virtual functions, but destructor is not virtual.
    /w14555   # expression with no effect or side-effect.
    /wd4100   # keep unused parameters.
    /MP       # build with multiple process.
    # /EHsc-    # no exception.
    /permissive-
  )
  
  if(NOT OPT_ENABLE_RTTI)
    list(APPEND CXX_FLAGS /GR-)
  endif()

  list(APPEND CXX_FLAGS_DEBUG           /Zi)
  list(APPEND CXX_FLAGS_RELEASE         /Ox /INCREMENTAL:NO /DEBUG:NONE /DNDEBUG)
  list(APPEND CXX_FLAGS_RELWITHDEBINFO  ${CXX_FLAGS_RELEASE} /Ox /Zi)
 
  list(APPEND CustomDefinitions 
   -DNOMINMAX
   -DWIN32
   -D_WINDOWS
   -D_CRT_SECURE_NO_WARNINGS
   -D_HAS_EXCEPTIONS=0
   #-DVC_EXTRALEAN
   #-DWIN32_LEAN_AND_MEAN
   -D_USE_MATH_DEFINES
  )

  list(APPEND CustomLinkFlags 
    ""
  )
endif()

# -----------------------------------------------------------------------------
# Third parties.
# -----------------------------------------------------------------------------

# Defines LIBM_LIBRARIES for mathematics, if needed.
helpers_findLibM()
list(APPEND CustomLibs ${LIBM_LIBRARIES})

if(OS_ANDROID)

  # [Work In Progress]

  # To compile for Android, run CMake with :
  # -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DANDROID_NATIVE_API_LEVEL=24

  # (OpenGL setting example from Android NDK samples). 
  # if (${ANDROID_PLATFORM_LEVEL} LESS 12)
  #   message(FATAL_ERROR "OpenGL 2 is not supported before API level 11 \
  #                       (currently using ${ANDROID_PLATFORM_LEVEL}).")
  #   return()
  # elseif (${ANDROID_PLATFORM_LEVEL} LESS 18)
  #   add_definitions("-DDYNAMIC_ES3")
  #   set(GL3STUB_SRC gl3stub.c)
  #   set(OPENGL_LIB GLESv2)
  # else ()
  #   set(OPENGL_LIB GLESv3)
  # endif (${ANDROID_PLATFORM_LEVEL} LESS 12)

  if (${ANDROID_PLATFORM_LEVEL} LESS 18)
    message(FATAL_ERROR "OpenGL ES 3 is not supported before API level 18 \
                        (currently using ${ANDROID_PLATFORM_LEVEL}).")
  else()
    set(OPENGL_LIBRARIES EGL) #
  endif()

  # Android logger.
  list(APPEND CustomLibs log)

else()
  
  # OpenGL
  find_package(OpenGL REQUIRED)

  # Extensions loader.
  if(OPT_USE_GLEW)
    find_package(GLEW 1.13 REQUIRED)
    if(${GLEW_FOUND})
      list(APPEND CustomDefinitions -DUSE_GLEW=1)
    endif()
  else()
    # Automatically generate header to load OpenGL extensions.
    find_package(PythonInterp)
    if(PYTHONINTERP_FOUND AND PYTHON_EXECUTABLE)
      set(GLEXTGEN_PATH "${BARBU_TOOLS_DIR}/glextgen")
      set(GLEXTGEN_OUTPUT_DIR "${BARBU_SOURCE_DIR}/core/impl/opengl")
      list(APPEND GLEXTGEN_OUTPUTS
        ${GLEXTGEN_OUTPUT_DIR}/_extensions.h
        ${GLEXTGEN_OUTPUT_DIR}/_extensions.inl
      )
      add_custom_command(
        OUTPUT
          ${GLEXTGEN_OUTPUTS}
        COMMAND
          ${PYTHON_EXECUTABLE} "${GLEXTGEN_PATH}/main.py"
                               "${GLEXTGEN_PATH}/extensions"
                               "${GLEXTGEN_OUTPUT_DIR}"
        DEPENDS
          "${GLEXTGEN_PATH}/main.py"
          "${GLEXTGEN_PATH}/extensions"
        WORKING_DIRECTORY
          ${CMAKE_SOURCE_DIR}
        COMMENT
          "Generate OpenGL extensions headers." VERBATIM
      )
      add_custom_target(OpenGLExtensionGenerator
        ALL
        DEPENDS
          ${GLEXTGEN_OUTPUTS}
        SOURCES
          "${GLEXTGEN_PATH}/main.py"
          "${GLEXTGEN_PATH}/extensions"
      )
    else()
      message(FATAL_ERROR "You need either the Python interpreter "
                          "or the GLEW library to handle OpenGL extensions.")
    endif()
    list(APPEND ADDITIONAL_MAKE_CLEAN_FILES "${GLEXTGEN_OUTPUT_DIR}")
  endif(OPT_USE_GLEW)

  # OpenMP
  if (OPT_USE_OPENMP)
    find_package(OpenMP)
    if(OPENMP_FOUND)
      list(APPEND CXX_FLAGS       "${OpenMP_CXX_FLAGS}")
      list(APPEND CustomLibs      "${OpenMP_CXX_LIBRARIES}")
    endif()
  endif()

endif()

# -----------------------------------------------------------------------------
# Thirdparties submodule dependencies.
# -----------------------------------------------------------------------------

#-------------------------
# GLM : math algebra.
#-------------------------
set(GLM_INCLUDE_DIR ${BARBU_THIRD_PARTY_DIR}/glm)
list(APPEND CustomDefinitions -DGLM_COMPILER_GCC61=0)

#-------------------------
# dear imgui : user interface.
#-------------------------
set(IMGUI_INCLUDE_DIR ${BARBU_THIRD_PARTY_DIR}/imgui)
list(APPEND IMGUI_SOURCES 
  ${IMGUI_INCLUDE_DIR}/imgui.cpp
  ${IMGUI_INCLUDE_DIR}/imgui_draw.cpp
  ${IMGUI_INCLUDE_DIR}/imgui_widgets.cpp
  ${IMGUI_INCLUDE_DIR}/imgui_tables.cpp
)
set(IMGUI_LIB imgui)
add_library(${IMGUI_LIB} ${BARBU_LIB_TYPE} ${IMGUI_SOURCES})
target_compile_options(
  ${IMGUI_LIB}
  PRIVATE
    ""
    "$<$<CONFIG:Debug>:>"
    "$<$<CONFIG:Release>:>"
    "$<$<CONFIG:DebugWithRelInfo>:>"
)
if (NOT ("${CustomLinkFlags}" MATCHES ""))
  set_target_properties(${IMGUI_LIB} PROPERTIES LINK_FLAGS ${CustomLinkFlags})
endif()

#-------------------------
# MikkTSpace : tangent-space calculation.
#-------------------------
set(MIKKTSPACE_INCLUDE_DIR ${BARBU_THIRD_PARTY_DIR}/MikkTSpace)
list(APPEND MIKKTSPACE_SOURCES
  ${MIKKTSPACE_INCLUDE_DIR}/mikktspace.c
  ${MIKKTSPACE_INCLUDE_DIR}/mikktspace.h
)
set(MIKKTSPACE_LIB MikkTSpace)
add_library(${MIKKTSPACE_LIB} ${BARBU_LIB_TYPE} ${MIKKTSPACE_SOURCES})
target_compile_options(
  ${MIKKTSPACE_LIB}
  PRIVATE
    ""
    "$<$<CONFIG:Debug>:>"
    "$<$<CONFIG:Release>:>"
    "$<$<CONFIG:DebugWithRelInfo>:>"
)
if (NOT ("${CustomLinkFlags}" MATCHES ""))
  set_target_properties(${MIKKTSPACE_LIB} PROPERTIES LINK_FLAGS ${CustomLinkFlags})
endif()

if (NOT OS_ANDROID)
  #-------------------------
  # GLFW : window manager.
  #-------------------------
  set(GLFW_ROOT ${BARBU_THIRD_PARTY_DIR}/glfw)
  set(GLFW_LIBRARY glfw)
  set(GLFW_INCLUDE_DIR ${GLFW_ROOT}/include)
  if(NOT EXISTS ${GLFW_INCLUDE_DIR})
    message(FATAL_ERROR "A third party was not found, please initialize git submodules via the commands : \n"
                        "git submodule init\n"
                        "git submodule update"
    )
  endif()
  set(BUILD_SHARED_LIBS   OPT_BUILD_SHARED_LIBS CACHE BOOL "Build GLFW as shared library.")
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Do not build GLFW examples.")
  set(GLFW_BUILD_TESTS    OFF CACHE BOOL "Do not run GLFW tests.")
  set(GLFW_BUILD_DOCS     OFF CACHE BOOL "Do not build GLFW documentation.")
  set(GLFW_INSTALL        OFF CACHE BOOL "Do not generate install targets.")
  add_subdirectory(${GLFW_ROOT} glfw EXCLUDE_FROM_ALL)
endif()

# Add the libraries as dependencies.
list(APPEND CustomLibs
  ${STD_CXX_FILESYSTEM_LIB}
  ${GLFW_LIBRARY}
  ${IMGUI_LIB}
  ${MIKKTSPACE_LIB}
)

# Third party sources dependencies to compile along the executable. 
# (header-only libs are optionals and could be added here for IDE integration).
set(IM3D_INCLUDE_DIR ${BARBU_THIRD_PARTY_DIR}/im3d)
list(APPEND Thirdparties
  ${IM3D_INCLUDE_DIR}/im3d.cpp
  ${IM3D_INCLUDE_DIR}/im3d.h
)

# -----------------------------------------------------------------------------
# System dependencies.
# -----------------------------------------------------------------------------

if(OS_LINUX)
  include(FindPkgConfig)

  # On GNU / Linux, we generate an app shortcut to be executed using the WM.
  helpers_generateUnixShortcut(${BARBU_BINARY_DIR})
elseif(OS_WIN32)
  # empty
elseif(OS_ANDROID)
  # empty
endif()

# -----------------------------------------------------------------------------
# Project dependencies settings.
# -----------------------------------------------------------------------------

# Include directories.
list(APPEND CustomIncludeDirs
  ${BARBU_SOURCE_DIR}
  ${BARBU_THIRD_PARTY_DIR}
  ${BARBU_SHADERS_DIR}
  
  ${GLFW_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
  ${GLM_INCLUDE_DIR}

  # OpenGL headers, needed on Windows.
  ${BARBU_THIRD_PARTY_DIR}/include
)

# Libraries.
list(APPEND CustomLibs
  ${GLFW_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES}
)

# Definitions.
list(APPEND CustomDefinitions 
  -DSHADERS_DIR="${BARBU_SHADERS_DIR}"
  -DASSETS_DIR="${BARBU_ASSETS_DIR}"
  -DBARBU_NPROC_MAX=${BARBU_NPROC_MAX}
)
if(OPT_ENABLE_DEBUG_LOG)
  list(APPEND CustomDefinitions 
    -DBARBU_ENABLE_DEBUG_LOG=1
  )
endif()

# Transform the linker flags from a list to a string to be accepted by set_target_properties LINK_FLAG.
# On CMake 3.13+, we could use LINK_OPTIONS instead.
set(LinkFlagsString "")
foreach(flag ${CustomLinkFlags})
  set(LinkFlagsString "${LinkFlagsString} ${flag}")
endforeach()

# -----------------------------------------------------------------------------
# Frameworks library.
# -----------------------------------------------------------------------------

add_subdirectory(${BARBU_SOURCE_DIR})

# -----------------------------------------------------------------------------
