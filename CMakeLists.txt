project (compiz)

set (CMAKE_MODULE_PATH_ORIG ${CMAKE_MODULE_PATH})
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
include (CompizCommon)
include (CompizPackage)
include (cmake/base.cmake)
include (CheckFunctionExists)


set (CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRe" CACHE INTERNAL "" FORCE)
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug/Release/RelWithDebInfo/MinSizeRe)" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# compiz package version number
# An odd micro number indicates in-progress development.
# An even micro number indicates a released version.
set (COMPIZ_VERSION_MAJOR 0)
set (COMPIZ_VERSION_MINOR 9)
set (COMPIZ_VERSION_MICRO 0)
set (VERSION ${COMPIZ_VERSION_MAJOR}.${COMPIZ_VERSION_MINOR}.${COMPIZ_VERSION_MICRO})

set (DECOR_INTERFACE_VERSION 20080901)

set (prefix ${CMAKE_INSTALL_PREFIX})
set (exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set (libdir ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
set (includedir ${CMAKE_INSTALL_PREFIX}/include)
set (datadir ${CMAKE_INSTALL_PREFIX}/share)

set (compiz_plugindir ${libdir}/compiz)
set (compiz_imagedir ${datadir}/compiz)
set (compiz_metadatadir ${datadir}/compiz)
set (COMPIZ_I18N_DIR ${CMAKE_SOURCE_DIR}/po)

set (ALL_LINGUAS af ar bg bn bn_IN bs ca cs cy da de el en_GB en_US es eu et fi fr gl gu he hi hr hu id it ja ka km ko lo lt mk mr nb nl or pa pl pt pt_BR ro ru sk sl sr sv ta tr uk vi xh zh_CN zh_TW zu)
set (GETTEXT_PACKAGE compiz)

find_package (Boost)
if (NOT Boost_FOUND OR Boost_MAJOR_VERSION EQUAL 1 AND Boost_MINOR_VERSION LESS 34)
    message (FATAL_ERROR "Boost version >= 1.34.0 needed")
endif (NOT Boost_FOUND OR Boost_MAJOR_VERSION EQUAL 1 AND Boost_MINOR_VERSION LESS 34)

set (COMPIZ_REQUIRES
    xdamage
    xcomposite
    x11-xcb
    xrandr
    xinerama
    ice
    sm
    libxml-2.0
    libxslt
    "libstartup-notification-1.0 >= 0.7"
)

set (DECORATION_REQUIRES xrender)

compiz_pkg_check_modules (COMPIZ REQUIRED ${COMPIZ_REQUIRES})
compiz_pkg_check_modules (LIBDECORATION REQUIRED ${DECORATION_REQUIRES})

include (CompizDefaults)

set (COMPIZ_CFLAGS "${COMPIZ_CFLAGS} -I${Boost_INCLUDE_DIR}")

set (_compiz_package_string "Compiz ${VERSION}")
set (_compiz_package "Compiz")

compiz_configure_file (${compiz_SOURCE_DIR}/config.h.core.in ${compiz_BINARY_DIR}/generated/config.h)

set (_commonfile "${CMAKE_BINARY_DIR}/generated/compiz-common.h")

file (WRITE  ${_commonfile} "#ifndef COMPIZ_COMMON_H\n")
file (APPEND ${_commonfile} "#define COMPIZ_COMMON_H\n")
file (APPEND ${_commonfile} "\n")
file (APPEND ${_commonfile} "#define COMPIZ_VERSION_MAJOR ${COMPIZ_VERSION_MAJOR}\n")
file (APPEND ${_commonfile} "#define COMPIZ_VERSION_MINOR ${COMPIZ_VERSION_MINOR}\n")
file (APPEND ${_commonfile} "#define COMPIZ_VERSION_MICRO ${COMPIZ_VERSION_MICRO}\n")
file (APPEND ${_commonfile} "\n")
file (APPEND ${_commonfile} "#define COMPIZ_VERSION_STRING \"${VERSION}\"\n")
file (APPEND ${_commonfile} "#endif\n")

install (
    FILES ${_commonfile}
    DESTINATION ${includedir}/compiz
)

compiz_configure_file (
    ${CMAKE_SOURCE_DIR}/compiz.pc.in
    ${CMAKE_BINARY_DIR}/compiz.pc
    COMPIZ_REQUIRES
    COMPIZ_CFLAGS
)

install (
    FILES ${CMAKE_BINARY_DIR}/compiz.pc
    DESTINATION ${libdir}/pkgconfig
)

add_subdirectory (cmake)
add_subdirectory (include)
add_subdirectory (images)
add_subdirectory (libdecoration)
add_subdirectory (gtk)
add_subdirectory (kde)
add_subdirectory (po)
add_subdirectory (metadata)
add_subdirectory (src)
add_subdirectory (xslt)
add_subdirectory (plugins)

compiz_package_generation ("Compiz")
compiz_add_uninstall ()

_print_configure_results ()

_check_compiz_cmake_macro (${CMAKE_MODULE_PATH_ORIG})


