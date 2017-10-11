project(bashtrace_wrapper)

set(SOURCES bashtrace_wrapper.c)

add_executable(${PROJECT_NAME} ${SOURCES})

configure_file(bashtrace.sh ${CMAKE_BINARY_DIR}/bashtrace.sh COPYONLY)

