
cmake_minimum_required (VERSION 3.1)
project (DirSurveiller)
# The version number.
set (DirSurveiller_VERSION_MAJOR 0)
set (DirSurveiller_VERSION_MINOR 0)
set (DirSurveiller_VERSION_PATCH 1)

enable_testing()

#Create directory for logs
set(LOG_DIR "${PROJECT_BINARY_DIR}/logs" CACHE PATH
    "Directory where logs are stored")
add_custom_target(build-time-make-log_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LOG_DIR})

#Create database directory if it isn't already build
set(DATABASE_DIR "${PROJECT_BINARY_DIR}/database" CACHE PATH
    "Directory that will contain the database")
add_custom_target(build-time-make-database_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DATABASE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )
IF( EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
    EXECUTE_PROCESS( COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
    )
ENDIF()


set(INSTALL_FOLDER "${PROJECT_BINARY_DIR}")
# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
    "${PROJECT_SOURCE_DIR}/src/DirSurveillerConfig.h.in"
    "${PROJECT_BINARY_DIR}/DirSurveillerConfig.h"
  )

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")

# putting all executable into bin


# should we include Debug Output?
option (DEBUG
        "Run programs in DEBUG MODE" OFF)

#ExternalProject Module for BerkleyDB
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
#SubDirectory
add_subdirectory(${PROJECT_SOURCE_DIR}/src/tracer)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/watchdog)
