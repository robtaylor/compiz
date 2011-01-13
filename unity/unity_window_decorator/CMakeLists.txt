project (unity-window-decorator)

find_package (Compiz REQUIRED)

include (CompizCommon)
include (CompizPackage)

set (CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRe" CACHE INTERNAL "" FORCE)
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug/Release/RelWithDebInfo/MinSizeRe)" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# compiz package version number
# An odd micro number indicates in-progress development.
# An even micro number indicates a released version.
set (COMPIZ_VERSION_MAJOR 0)
set (COMPIZ_VERSION_MINOR 9)
set (COMPIZ_VERSION_MICRO 2)
set (COMPIZ_VERSION_MACRO 1)
set (VERSION ${COMPIZ_VERSION_MAJOR}.${COMPIZ_VERSION_MINOR}.${COMPIZ_VERSION_MICRO}.${COMPIZ_VERSION_MACRO})

option (BUILD_UNITY "Build Unity window decorator" 1)
option (BUILD_METACITY "Unity-window-decorator metacity theme support" 1)

compiz_set (USE_METACITY ${BUILD_METACITY})

add_subdirectory (src)

compiz_print_configure_header ("Compiz Unity Window Decorator")
compiz_color_message ("\n${_escape}[4mFeatures:${_escape}[0m\n")

compiz_print_result_message ("Metacity Theme Support" USE_METACITY)
compiz_print_configure_footer ()

compiz_add_uninstall ()
compiz_package_generation ("Compiz Unity Window Decorator")


