project(plugin)



configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/PluginConfig.h.in"
    "${CMAKE_BINARY_DIR}/PluginConfig.h"
  )

###### Require Pthreads ###########

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_library(Libaudit libaudit.a)
find_library(Libauparse libauparse.a)
set(queue_SOURCES queue.c)
add_library(queue STATIC ${queue_SOURCES})
set_property(TARGET queue PROPERTY C_STANDARD 99)

target_include_directories(queue
    PUBLIC include
)

set(config_SOURCES au-dirsurv-config.c)
add_library(disp_config STATIC ${config_SOURCES})
set_property(TARGET disp_config PROPERTY C_STANDARD 99)

target_include_directories(disp_config
    PUBLIC include
)

set(fgets_SOURCES remote-fgets.c)
add_library(fgets STATIC ${fgets_SOURCES})
set_property(TARGET fgets PROPERTY C_STANDARD 99)

target_include_directories(fgets
    PUBLIC include
)

#Executable

add_executable(au-dirsurv
    audisp-dirsurv.c)
set_property(TARGET au-dirsurv PROPERTY C_STANDARD 99)

target_link_libraries(au-dirsurv
    PRIVATE queue
    PRIVATE disp_config
    PRIVATE fgets
    ${Libauparse}
    ${Libaudit}
    Threads::Threads)
