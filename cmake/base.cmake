cmake_minimum_required (VERSION 2.4)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    message (SEND_ERROR "Building in the source directory is not supported.")
    message (FATAL_ERROR "Please remove the created \"CMakeCache.txt\" file, the \"CMakeFiles\" directory and create a build directory and call \"${CMAKE_COMMAND} <path to the sources>\".")
endif ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")

if (CMAKE_MAJOR_VERSION GREATER 2 OR CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION GREATER 5)
cmake_policy (VERSION 2.4)
cmake_policy (SET CMP0000 OLD)
cmake_policy (SET CMP0003 NEW)
cmake_policy (SET CMP0005 OLD)
endif (CMAKE_MAJOR_VERSION GREATER 2 OR CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION GREATER 5)

set (PKGCONFIG_REGEX ".*${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:${CMAKE_INSTALL_PREFIX}/share/pkgconfig.*")

# add install prefix to pkgconfig search path if needed
if (NOT "$ENV{PKG_CONFIG_PATH}" MATCHES "${PKGCONFIG_REGEX}")
    if ("" STREQUAL "$ENV{PKG_CONFIG_PATH}")
        set (ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:${CMAKE_INSTALL_PREFIX}/share/pkgconfig")
    else ("" STREQUAL "$ENV{PKG_CONFIG_PATH}")
        set (ENV{PKG_CONFIG_PATH}
             "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:${CMAKE_INSTALL_PREFIX}/share/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif ("" STREQUAL "$ENV{PKG_CONFIG_PATH}")
endif (NOT "$ENV{PKG_CONFIG_PATH}" MATCHES "${PKGCONFIG_REGEX}")

include (FindPkgConfig)
