cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

project(AD7705_RaspberryPi_Library VERSION 1.0.0 LANGUAGES CXX)
include(GNUInstallDirs)

add_subdirectory(example)

# add_compile_options(-Wall -Wconversion -Wextra -pedantic)

set(LIBSRC AD7705Comm.cpp)
set(LIBINCLUDE AD7705Comm.h)

add_library(ad7705
  SHARED
  ${LIBSRC}
  )

set_target_properties(ad7705 PROPERTIES
  SOVERSION 1
  PUBLIC_HEADER "${LIBINCLUDE}")

target_link_libraries(ad7705 ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ad7705 EXPORT ad7705-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})



add_library(ad7705_static
  STATIC
  ${LIBSRC}
  )

set_target_properties(ad7705_static PROPERTIES
  PUBLIC_HEADER "${LIBINCLUDE}")

target_link_libraries(ad7705_static pigpio)

install(TARGETS ad7705_static EXPORT lsm9ds1_static-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
