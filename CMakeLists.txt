cmake_minimum_required(VERSION 3.25)
project(jpeg_sample C)

set(CMAKE_C_STANDARD 11)

FILE(COPY_FILE test.jpg ${PROJECT_BINARY_DIR}/test.jpg)
message(STATUS "Start")

set(CMAKE_PREFIX_PATH /opt/libjpeg-turbo)
find_package(PkgConfig REQUIRED)
pkg_search_module(TURBOJPEG REQUIRED libturbojpeg)

if (NOT TURBOJPEG_FOUND)
    message(FATAL_ERROR "Cannot find jpeg")
endif ()

message(STATUS "Found jpeg")
message("turojpeg include dir is ${TURBOJPEG_INCLUDE_DIRS}")
message("turojpeg lib dir is ${TURBOJPEG_LIBDIR}")
message("turojpeg library is ${TURBOJPEG_LIBRARIES}")

link_directories(${TURBOJPEG_LIBDIR})

add_executable(jpeg_sample main.c)

target_include_directories(jpeg_sample PUBLIC ${TURBOJPEG_INCLUDE_DIRS})
target_link_libraries(jpeg_sample ${TURBOJPEG_LIBRARIES})