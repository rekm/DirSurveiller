project(watchdog)

enable_testing()

###### Require Pthreads ###########

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

###### Definition Library  ######

set(deflib_SOURCES
    src/definitions.c)

#Generate the static library from the library sources
add_library(def_library STATIC ${deflib_SOURCES})

set_property(TARGET def_library PROPERTY C_STANDARD 99)

target_include_directories(def_library
    PUBLIC include
)

###### Containers Library  ######

set(library_SOURCES
    src/containers.c)

#Generate the static library from the library sources
add_library(containers_library STATIC ${library_SOURCES})

set_property(TARGET containers_library PROPERTY C_STANDARD 99)

target_include_directories(containers_library
    PUBLIC include
)


####### Generate static library from libart dependency ######

set(libart_SOURCES
    deps/libart/src/art.c)

add_library(libart STATIC ${libart_SOURCES})

target_include_directories(libart
    PUBLIC deps/libart/src)

####### Berkley DB #######
ExternalProject_Add( db
    URL http://download.oracle.com/berkeley-db/db-6.2.32.tar.gz
    SOURCE_DIR ${CMAKE_BINARY_DIR}/db_static-src
    BINARY_DIR ${CMAKE_BINARY_DIR}/db_static-bin
    CONFIGURE_COMMAND ${CMAKE_BINARY_DIR}/db_static-src/dist/configure --disable-shared --prefix ${CMAKE_BINARY_DIR}/db_static-bin
    BUILD_COMMAND "make"
)

add_library(berkeleyDB STATIC IMPORTED)
set_property(TARGET berkeleyDB
    PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/db_static-bin/lib/libdb-6.2.a)
add_dependencies(berkeleyDB db)

###### Database Management Library  ######

set(db_man_SOURCES
    src/database.c)

#Generate the static library from the library sources
add_library(db_man STATIC ${db_man_SOURCES})
set_property(TARGET db_man PROPERTY C_STANDARD 99)

target_include_directories(db_man
    PUBLIC include
)
add_dependencies(db_man berkeleyDB)
target_link_libraries(db_man
    PRIVATE berkeleyDB
    PRIVATE containers_library)

add_executable(db_man_test src/database_test.c )
target_link_libraries(db_man_test
    PRIVATE containers_library
    PRIVATE db_man
    Threads::Threads)
add_test(DbManagerMarshallTestEcall1 ${CMAKE_BINARY_DIR}/db_man_test 1)
add_test(DbManagerMarshallTestOcall1 ${CMAKE_BINARY_DIR}/db_man_test 2)

####### Adding executables and setting library dependencies ####


add_executable(watchdog_daemon
    src/watchdog.c)
set_property(TARGET watchdog_daemon PROPERTY C_STANDARD 99)

add_executable(watchdog_ctrl
    src/watchdog_ctrl.c)
set_property(TARGET watchdog_ctrl PROPERTY C_STANDARD 99)

target_link_libraries(watchdog_daemon
    PRIVATE def_library
    PRIVATE berkeleyDB
    PRIVATE db_man
    PRIVATE containers_library
    PRIVATE libart
    Threads::Threads)

target_link_libraries(watchdog_ctrl
    PRIVATE db_man
    Threads::Threads)
