cmake_minimum_required(VERSION 3.7)
project(coiopp)

add_library(${PROJECT_NAME}
    libco/libco.c
    coio.cpp
    coio_glib.cpp)

add_custom_target(${PROJECT_NAME}_files SOURCES coioimpl.h)

set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 99
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON)

target_compile_options(${PROJECT_NAME} PRIVATE -W -Wall -Wextra -Werror)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GIO2 REQUIRED gio-2.0)

target_include_directories(${PROJECT_NAME} PUBLIC ${GLIB2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GLIB2_LIBRARIES})

target_include_directories(${PROJECT_NAME} PRIVATE ${GIO2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GIO2_LIBRARIES})

target_link_libraries(${PROJECT_NAME} m)

target_include_directories(${PROJECT_NAME} PUBLIC .)
