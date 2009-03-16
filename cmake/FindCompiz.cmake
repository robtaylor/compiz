# Find Compiz and provide all necessary variables and macros to compile software for it.

if (NOT COMPIZ_INTERNAL)

    include (FindPkgConfig)

    if (COMPIZ_FIND_REQUIRED)
	set (_req REQUIRED)
    endif ()

    set (PKGCONFIG_REGEX ".*\${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:\${CMAKE_INSTALL_PREFIX}/share/pkgconfig.*")

    # add install prefix to pkgconfig search path if needed
    if (NOT "$ENV{PKG_CONFIG_PATH}" MATCHES "${PKGCONFIG_REGEX}")
	if ("" STREQUAL "$ENV{PKG_CONFIG_PATH}")
	    set (ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:${CMAKE_INSTALL_PREFIX}/share/pkgconfig")
	else ()
	    set (ENV{PKG_CONFIG_PATH}
		"${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:${CMAKE_INSTALL_PREFIX}/share/pkgconfig:$ENV{PKG_CONFIG_PATH}")
	endif ()
    endif ()

    pkg_check_modules (COMPIZ ${_req} compiz)

    if (COMPIZ_FOUND)
	set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} ${COMPIZ_PREFIX}/share/compiz/cmake)
	include (CompizDefaults)
    endif ()
endif ()

