cmake_minimum_required(VERSION 3.5)
project(OSMPTraceFilePlayer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LINK_WITH_SHARED_OSI OFF CACHE BOOL "Link FMU with shared OSI library instead of statically linking")
set(PUBLIC_LOGGING_TRACE_FILE_PLAYER OFF CACHE BOOL "Enable logging via FMI logger")
set(PRIVATE_LOGGING_TRACE_FILE_PLAYER OFF CACHE BOOL "Enable private logging to file")
set(VERBOSE_FMI_LOGGING_TRACE_FILE_PLAYER OFF CACHE BOOL "Enable detailed FMI function logging")
set(DEBUG_BREAKS_TRACE_FILE_PLAYER OFF CACHE BOOL "Enable debugger traps for debug builds of FMU")

if(PRIVATE_LOGGING_TRACE_FILE_PLAYER)
  if(WIN32)
    set(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER "C:/TEMP/OSMPTraceFilePlayerLog.log" CACHE FILEPATH "Path to write private log file to")
  else()
    set(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER "/tmp/OSMPTraceFilePlayerLog.log" CACHE FILEPATH "Path to write private log file to")
  endif()
  file(TO_NATIVE_PATH ${PRIVATE_LOG_PATH_TRACE_FILE_PLAYER} PRIVATE_LOG_PATH_TRACE_FILE_PLAYER_NATIVE)
  string(REPLACE "\\" "\\\\" PRIVATE_LOG_PATH_TRACE_FILE_PLAYER ${PRIVATE_LOG_PATH_TRACE_FILE_PLAYER_NATIVE})
endif()

# Set a default build type if none was specified
set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/../.git")
  set(default_build_type "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(OSMPVERSION "1.2.0" CACHE STRING "OSMP Version String")

add_subdirectory( lib/open-simulation-interface )

get_directory_property(OSI_VERSION_MAJOR DIRECTORY lib/open-simulation-interface DEFINITION VERSION_MAJOR)
get_directory_property(OSI_VERSION_MINOR DIRECTORY lib/open-simulation-interface DEFINITION VERSION_MINOR)
get_directory_property(OSI_VERSION_PATCH DIRECTORY lib/open-simulation-interface DEFINITION VERSION_PATCH)
set(OSIVERSION "${OSI_VERSION_MAJOR}.${OSI_VERSION_MINOR}.${OSI_VERSION_PATCH}")

include_directories( lib/fmi2/headers )
add_subdirectory( src )
