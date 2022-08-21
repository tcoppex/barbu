# -----------------------------------------------------------------------------
# Helpers functions for CMake (2020 - unlicense.org)
# 
# -----------------------------------------------------------------------------

# Notes : This files prefers using "function" rather than "macros".
#         Hence if we want to pass a value to the parent scope we
#         need to call set($(varname) $(varname) PARENT_SCOPE) at the end.

# -----------------------------------------------------------------------------
# Capitalize first letter of a string.
# -----------------------------------------------------------------------------

function(helpers_capitalize input)
  string(SUBSTRING ${input} 0 1 FIRST_LETTER)
  string(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
  string(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" output "${input}")
  set(CAPITALIZE_OUTPUT ${output} PARENT_SCOPE) #
endfunction(helpers_capitalize)

# -----------------------------------------------------------------------------
# Generate a c++ configuration header from a template.
# -----------------------------------------------------------------------------

function(helpers_generateUnixShortcut output_dir)
  # Define variables that will be replaced in the configuration file.
  # Note : some of the used variables are already defined.
  #------------

  # already defined : BARBU_ASSETS_DIR
  set(BARBU_NAME          "${CMAKE_PROJECT_NAME}")
  set(BARBU_EXE_PATH      ${BARBU_BINARY_DIR}/${BARBU_NAME}) #

  helpers_capitalize(${BARBU_NAME})
  set(BARBU_APPNAME ${CAPITALIZE_OUTPUT})

  set(TEMPLATE_FILE       ${CMAKE_MODULE_PATH}/templates/app.desktop.in)
  set(SHORTCUT_DST_FILE   ${output_dir}/${BARBU_APPNAME}.desktop)
  configure_file(
    ${TEMPLATE_FILE} 
    ${SHORTCUT_DST_FILE}
    NEWLINE_STYLE UNIX
  )
  # Add the file to the clean target.
  if(EXISTS ${SHORTCUT_DST_FILE} AND NOT IS_DIRECTORY ${SHORTCUT_DST_FILE})
    set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES ${SHORTCUT_DST_FILE})
  endif()

  # Put the config file in the parent scope to be added to dependencies.
  set(SHORTCUT_DST_FILE ${SHORTCUT_DST_FILE} PARENT_SCOPE)
endfunction(helpers_generateUnixShortcut)

# -----------------------------------------------------------------------------
# Set a default build type if none was specified
# -----------------------------------------------------------------------------

function(helpers_setDefaultBuildType DEFAULT_BUILD_TYPE)  
  # Marcus D. Hanwell @ https://blog.kitware.com/cmake-and-the-default-build-type/
  # Default to Debug when used on a git clone.
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    #set(DEFAULT_BUILD_TYPE "Debug")
  endif()
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none were specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
        STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
      "Debug" "Release" "RelWithDebInfo")
  endif()
endfunction()

# -----------------------------------------------------------------------------
# Specify a global output directory.
# -----------------------------------------------------------------------------

function(helpers_setGlobalOutputDirectory OUTPUT_DIR)
  # Default output directory.
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR} CACHE PATH "")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR} CACHE PATH "")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR} CACHE PATH "")
endfunction(helpers_setGlobalOutputDirectory)

# -----------------------------------------------------------------------------
# Force a target to a specific output directory.
# -----------------------------------------------------------------------------

function(helpers_setTargetOutputDirectory target output_dir)
  # Force output directory destination, especially for MSVC (@so7747857).
  foreach(type RUNTIME LIBRARY ARCHIVE)
    set_target_properties(${target} PROPERTIES
      ${type}_OUTPUT_DIRECTORY         ${output_dir}
      ${type}_OUTPUT_DIRECTORY_DEBUG   ${output_dir}
      ${type}_OUTPUT_DIRECTORY_RELEASE ${output_dir}
    )
  endforeach()
endfunction(helpers_setTargetOutputDirectory)

# -----------------------------------------------------------------------------
#  Setup clang-tidy.
# -----------------------------------------------------------------------------

function(helpers_setClangTidy check_params filter_params)
  if(NOT CLANG_TIDY_EXE)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy") 
  endif()
  if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE} --checks=${check_params} --header-filter=${filter_params})
    set(CMAKE_CXX_CLANG_TIDY ${CMAKE_CXX_CLANG_TIDY} PARENT_SCOPE)
  endif()
  message(STATUS "Check for clang-tidy : ${CLANG_TIDY_EXE}")
endfunction()

# -----------------------------------------------------------------------------
# Check compiler version.
# -----------------------------------------------------------------------------

# Check the C and CXX compiler are identical and
# matched the required minimum version.
function(helpers_checkCompilerVersion COMPILER_ID MIN_VERSION_REQUIRED)
  # Check we use the same compiler family for both C and CXX.
  if(   (CMAKE_C_COMPILER_ID MATCHES COMPILER_ID) 
    AND (CMAKE_CXX_COMPILER_ID MATCHES COMPILER_ID))
    # C version
    if(CMAKE_C_COMPILER_VERSION VERSION_LESS MIN_VERSION_REQUIRED)
      message(
        FATAL_ERROR 
        "Detected C compiler ${COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} < ${MIN_VERSION_REQUIRED}."
      )
    endif()
    # CXX version
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_VERSION_REQUIRED)
      message(
        FATAL_ERROR 
        "Detected CXX compiler ${COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} < ${MIN_VERSION_REQUIRED}."
      )
    endif()
  elseif()
    message(
      FATAL_ERROR
      "Detected C and CXX compilers does not match ID ${COMPILER_ID}."
    )
  endif()
endfunction(helpers_checkCompilerVersion)

# -----------------------------------------------------------------------------
# Setup LibM(athematics) if needed.
# -----------------------------------------------------------------------------

function(helpers_findLibM)
  # Chuck Atkins @ https://cmake.org/pipermail/cmake/2019-March/069168.html
  include(CheckCSourceCompiles)
  set(LIBM_TEST_SOURCE "#include<math.h>\nint main(){float f = sqrtf(4.0f);}")
  check_c_source_compiles("${LIBM_TEST_SOURCE}" HAVE_MATH)
  if(HAVE_MATH)
    set(LIBM_LIBRARIES)
  else()
    set(CMAKE_REQUIRED_LIBRARIES m)
    check_c_source_compiles("${LIBM_TEST_SOURCE}" HAVE_LIBM_MATH)
    unset(CMAKE_REQUIRED_LIBRARIES)
    if(NOT HAVE_LIBM_MATH)
      message(FATAL_ERROR "Unable to use C math library functions.")
    endif()
    set(LIBM_LIBRARIES m)
  endif()
endfunction(helpers_findLibM)

# -----------------------------------------------------------------------------
