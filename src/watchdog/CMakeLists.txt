project(watchdog)


###### Require Pthreads ###########

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

###### Containers Library  ######

set(library_SOURCES
    src/containers.c)

#Generate the static library from the library sources
add_library(containers_library STATIC ${library_SOURCES})

set_property(TARGET containers_library PROPERTY C_STANDARD 99)

target_include_directories(containers_library
    PUBLIC include
)


#Generate static library from libart dependency

set(libart_SOURCES
    deps/libart/src/art.c)

add_library(libart STATIC ${libart_SOURCES})

target_include_directories(libart
    PUBLIC deps/libart/src)

add_executable(watchdog_daemon
    src/watchdog.c)

set_property(TARGET watchdog_daemon PROPERTY C_STANDARD 99)

add_executable(watchdog_ctrl
    src/watchdog_ctrl.c)

set_property(TARGET watchdog_ctrl PROPERTY C_STANDARD 99)

target_link_libraries(watchdog_daemon
    PRIVATE containers_library
    PRIVATE libart
    Threads::Threads)
