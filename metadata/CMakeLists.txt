set (_plugin_files
    core
    annotate
    blur
    clone
    cube
    dbus
    decor
    fade
    fs
    gconf
    glib
    ini
    inotify
    minimize
    move
    place
    png
    regex
    resize
    rotate
    scale
    screenshot
    svg
    switcher
    video
    water
    wobbly
    zoom
    composite
    opengl
    copytex
)

set (_xml_targets)
set (_schema_targets)

foreach (_plugin ${_plugin_files})

    translate_xml (
	${CMAKE_CURRENT_SOURCE_DIR}/${_plugin}.xml.in
	${CMAKE_CURRENT_BINARY_DIR}/${_plugin}.xml
    )

    install (
	FILES ${CMAKE_CURRENT_BINARY_DIR}/${_plugin}.xml
	DESTINATION ${compiz_metadatadir}
    )

    if (USE_GCONF)
	generate_gconf_schema (
	    ${CMAKE_CURRENT_BINARY_DIR}/${_plugin}.xml
	    ${CMAKE_CURRENT_BINARY_DIR}/compiz-${_plugin}.schemas
	)
	install_gconf_schema (${CMAKE_CURRENT_BINARY_DIR}/compiz-${_plugin}.schemas)
	set (_schema_targets ${_schema_targets} ${CMAKE_CURRENT_BINARY_DIR}/compiz-${_plugin}.schemas)
    endif (USE_GCONF)

    set (_xml_targets ${_xml_targets} ${CMAKE_CURRENT_BINARY_DIR}/${_plugin}.xml)
endforeach (_plugin)

add_custom_target (xml-files ALL
    DEPENDS ${_xml_targets}
)

if (USE_GCONF)
    add_custom_target (gconf-schemas ALL
	DEPENDS ${_schema_targets}
    )
endif (USE_GCONF)