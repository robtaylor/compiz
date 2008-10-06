if (USE_GTK)

    include_directories (
	${compiz_SOURCE_DIR}/include
	${compiz_BINARY_DIR}
	${GTK_WINDOW_DECORATOR_INCLUDE_DIRS}
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
	${COMPIZ_LINK_DIRS}
    )

    if (USE_GCONF)
        set (gwd_schema ${CMAKE_CURRENT_BINARY_DIR}/gwd.schemas)
        translate_xml (
	    ${CMAKE_CURRENT_SOURCE_DIR}/gwd.schemas.in
	    ${gwd_schema}
	)
	install_gconf_schema (${CMAKE_CURRENT_BINARY_DIR}/gwd.schemas)
    endif (USE_GCONF)

    add_executable (gtk-window-decorator
	gtk-window-decorator.c
	${gwd_schema}
    )

    add_dependencies (gtk-window-decorator decoration)

    if (USE_METACITY)
	set (metacitylibs ${METACITY_LIBRARIES})
    endif (USE_METACITY)


    target_link_libraries (
	gtk-window-decorator
	decoration
	${GTK_WINDOW_DECORATOR_LIBRARIES}
	${GCONF_LIBRARIES}
	${DBUS_GLIB_LIBRARIES}
	${metacitylibs}
    )

    install (
	TARGETS gtk-window-decorator
	DESTINATION ${exec_prefix}
    )


endif (USE_GTK)



