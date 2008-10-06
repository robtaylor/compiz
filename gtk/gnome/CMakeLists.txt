
if (USE_GNOME)

    include_directories (
	${compiz_BINARY_DIR}
	${GNOME_WINDOW_SETTINGS_INCLUDE_DIRS}
    )

    add_definitions (
	-DHAVE_CONFIG_H
	-DMETACITY_THEME_DIR=\\\"${windowsettingsdatadir}/themes\\\"
    )

    link_directories (
	${COMPIZ_LINK_DIRS}
    )

    set (desktop_file ${CMAKE_CURRENT_BINARY_DIR}/compiz.desktop)
    translate_desktop_file (
        ${CMAKE_CURRENT_SOURCE_DIR}/compiz.desktop.in
        ${desktop_file}
    )
    install (
	FILES ${desktop_file}
	DESTINATION ${windowsettingsdatadir}/gnome/wm-properties
    )

    if (USE_GNOME_KEYBINDINGS)
        translate_xml (
	    ${CMAKE_CURRENT_SOURCE_DIR}/50-compiz-desktop-key.xml.in
	    ${CMAKE_CURRENT_BINARY_DIR}/50-compiz-desktop-key.xml
	)
	translate_xml (
	    ${CMAKE_CURRENT_SOURCE_DIR}/50-compiz-key.xml.in
	    ${CMAKE_CURRENT_BINARY_DIR}/50-compiz-key.xml
	)
	set (key_xml
	    ${CMAKE_CURRENT_BINARY_DIR}/50-compiz-desktop-key.xml
	    ${CMAKE_CURRENT_BINARY_DIR}/50-compiz-key.xml
	)
	install (
	    FILES ${key_xml}
	    DESTINATION ${keybindingsdir}
	)
    endif (USE_GNOME_KEYBINDINGS)

    add_library (gnome-compiz MODULE
	compiz-window-manager.c
	compiz-window-manager.h
	${key_xml}
	${desktop_file}
    )

    set_target_properties (gnome-compiz
	PROPERTIES OUTPUT_NAME compiz
    )

    target_link_libraries (
	gnome-compiz ${GNOME_WINDOW_SETTINGS_LIBRARIES}
    )

    install (
	TARGETS gnome-compiz
	DESTINATION ${windowsettingslibdir}/window-manager-settings
    )

endif (USE_GNOME)
