# Find Compiz and provide all necessary variables and macros to compile software for it.

if (NOT COMPIZ_INTERNAL)

    if (Compiz_FIND_REQUIRED)
	set (_req REQUIRED)
    endif ()

    find_package (PkgConfig ${_req})

    if (PKG_CONFIG_FOUND)

	set (_comp_ver)
	if (Compiz_FIND_VERSION)
	    if (Compiz_FIND_VERSION_EXACT)
		set (_comp_ver "=${Compiz_FIND_VERSION}")
	    else ()
		set (_comp_ver ">=${Compiz_FIND_VERSION}")
	    endif ()
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

	pkg_check_modules (COMPIZ ${_req} "compiz${_comp_ver}")

	find_path(_compiz_def_macro CompizDefaults.cmake ${COMPIZ_PREFIX}/share/compiz/cmake)

	if (COMPIZ_FOUND AND _compiz_def_macro)
	    set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} ${COMPIZ_PREFIX}/share/compiz/cmake)
	    include (CompizDefaults)
	elseif (Compiz_FIND_REQUIRED)
	    message (FATAL_ERROR "Unable to find Compiz ${_comp_ver}")
	endif ()
    endif ()
endif ()

