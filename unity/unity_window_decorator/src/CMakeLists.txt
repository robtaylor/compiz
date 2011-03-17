function (compiz_install_gconf_schema _src _dst)
    pkg_check_modules (GCONF gconf-2.0)
    find_program (GCONFTOOL_EXECUTABLE gconftool-2)
    mark_as_advanced (FORCE GCONFTOOL_EXECUTABLE)

    if (GCONFTOOL_EXECUTABLE AND NOT COMPIZ_DISABLE_SCHEMAS_INSTALL)
	install (CODE "
		if (\"\$ENV{USER}\" STREQUAL \"root\")
		    exec_program (${GCONFTOOL_EXECUTABLE}
			ARGS \"--get-default-source\"
			OUTPUT_VARIABLE ENV{GCONF_CONFIG_SOURCE})
		    exec_program (${GCONFTOOL_EXECUTABLE}
			ARGS \"--makefile-install-rule ${_src} > /dev/null\")
		else (\"\$ENV{USER}\" STREQUAL \"root\")
		    exec_program (${GCONFTOOL_EXECUTABLE}
			ARGS \"--install-schema-file=${_src} > /dev/null\")
		endif (\"\$ENV{USER}\" STREQUAL \"root\")
		")
    endif ()
    install (
	FILES "${_src}"
	DESTINATION "${COMPIZ_DESTDIR}${_dst}"
    )
endfunction ()

set (USE_GCONF_UNITY_WINDOW_DECORATOR 1 CACHE BOOL "Install GConf schemas for Unity Window Decorator")

if (BUILD_UNITY)

    pkg_check_modules (UNITY_WINDOW_DECORATOR
	gconf-2.0
	xrender>=0.8.4
	gtk+-2.0>=2.18.0
	libwnck-1.0
	pangocairo
    )
    if (UNITY_WINDOW_DECORATOR_FOUND)
	include (CheckFunctionExists)
        set (CMAKE_REQUIRED_FLAGS ${UNITY_WINDOW_DECORATOR_CFLAGS})
        set (CMAKE_REQUIRED_LIBRARIES ${UNITY_WINDOW_DECORATOR_LIBRARIES})
        list (FIND CMAKE_REQUIRED_FLAGS "-D_REENTRANT" REENTRANT_INDEX)
        if (REENTRANT_INDEX)
          list (REMOVE_AT CMAKE_REQUIRED_FLAGS REENTRANT_INDEX)
          list (APPEND CMAKE_REQUIRED_FLAGS "-D_REENTRANT=1")
        endif (REENTRANT_INDEX)
        check_function_exists (wnck_window_has_name HAVE_WNCK_WINDOW_HAS_NAME)
        set (CMAKE_REQUIRED_FLAGS "")
        set (CMAKE_REQUIRED_LIBRARIES "")

        compiz_pkg_check_modules (HAVE_LIBWNCK_2_18_1 libwnck-1.0>=2.18.1)
        compiz_pkg_check_modules (HAVE_LIBWNCK_2_19_4 libwnck-1.0>=2.19.4)

        if (BUILD_METACITY)
            pkg_check_modules (METACITY libmetacity-private)
            if (METACITY_FOUND)
		compiz_pkg_check_modules (HAVE_METACITY_2_15_21 libmetacity-private>=2.15.21)
		compiz_pkg_check_modules (HAVE_METACITY_2_17_0 libmetacity-private>=2.17.0)
		compiz_pkg_check_modules (HAVE_METACITY_2_23_2 libmetacity-private>=2.23.2)
            else (METACITY_FOUND)
		compiz_set (USE_METACITY 0)
            endif (METACITY_FOUND)
        endif (BUILD_METACITY)

	if (COMPIZ_BUILD_WITH_RPATH)
	    set (CMAKE_INSTALL_RPATH ${libdir})
	endif (COMPIZ_BUILD_WITH_RPATH)

	configure_file (
	    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.gtk.in
	    ${CMAKE_CURRENT_BINARY_DIR}/config.h
	)

	include_directories (
        ${compiz_SOURCE_DIR}/include
	    ${CMAKE_CURRENT_BINARY_DIR}
	    ${UNITY_WINDOW_DECORATOR_INCLUDE_DIRS}
	    ${METACITY_INCLUDE_DIRS}
	    ${GCONF_INCLUDE_DIRS}
	    ${DBUS_GLIB_INCLUDE_DIRS}
	)

	add_definitions (
	    -DHAVE_CONFIG_H
	    -DALL_LINGUAS=\"${ALL_LINGUAS}\"
	    -DLOCALEDIR=\\\"${datadir}/locale\\\"
	)

	link_directories (
	    ${UNITY_WINDOW_DECORATOR_LIBRARY_DIRS}
	    ${COMPIZ_LINK_DIRS}
	)

	if (USE_GCONF_UNITY_WINDOW_DECORATOR)

	    if (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
		set (SCHEMADIR "${CMAKE_INSTALL_PREFIX}/share/gconf/schemas")
	    else (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
		set (SCHEMADIR "${COMPIZ_INSTALL_GCONF_SCHEMA_DIR}")
	    endif (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)

	    set (gwd_schema ${CMAKE_CURRENT_BINARY_DIR}/gwd.schemas)

	    compiz_translate_xml (
	        ${CMAKE_CURRENT_SOURCE_DIR}/gwd.schemas.in
	        ${gwd_schema}
	    )

	    compiz_install_gconf_schema (${CMAKE_CURRENT_BINARY_DIR}/gwd.schemas ${SCHEMADIR})

	endif (USE_GCONF_UNITY_WINDOW_DECORATOR)

	add_executable (unity-window-decorator
			gtk-window-decorator.c
			blurprops.c
			decorprops.c
			cairo.c
			gdk.c
			switcher.c
			metacity.c
			events.c
			forcequit.c
			actionmenu.c
			settings.c
			util.c
			style.c
			wnck.c
			decorator.c
			${gwd_schema}
	)

	if (USE_METACITY)
	    set (metacitylibs ${METACITY_LIBRARIES})
	endif (USE_METACITY)

	set_target_properties (
	    unity-window-decorator PROPERTIES
	    INSTALL_RPATH_USE_LINK_PATH 0
	)

	target_link_libraries (unity-window-decorator
				decoration
				${UNITY_WINDOW_DECORATOR_LIBRARIES}
				${GCONF_LIBRARIES}
				${DBUS_GLIB_LIBRARIES}
				${metacitylibs}
	)

	install (
	    TARGETS unity-window-decorator
	    DESTINATION ${COMPIZ_DESTDIR}${exec_prefix}
	    RUNTIME DESTINATION bin
	)

    else (UNITY_WINDOW_DECORATOR_FOUND)
	set (USE_UNITY 0)
    endif (UNITY_WINDOW_DECORATOR_FOUND)

endif (BUILD_UNITY)



