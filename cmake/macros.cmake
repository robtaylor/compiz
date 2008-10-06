
macro (check_pkg_module _module _var)
    if (NOT ${_var})
        pkg_check_modules (_${_var} ${_module})
	if (_${_var}_FOUND)
	    set (${_var} 1 CACHE INTERNAL "" FORCE)
	endif (_${_var}_FOUND)
	set(__pkg_config_checked__${_var} 0 CACHE INTERNAL "" FORCE)
    endif (NOT ${_var})
endmacro (check_pkg_module _module)

macro (translate_xml _src _dst)
    find_program (INTLTOOL_MERGE_EXECUTABLE intltool-merge)
    mark_as_advanced (FORCE INTLTOOL_MERGE_EXECUTABLE)

    if (INTLTOOL_MERGE_EXECUTABLE)
	add_custom_command (
	    OUTPUT ${_dst}
	    COMMAND ${INTLTOOL_MERGE_EXECUTABLE} -x -u -c
		    ${CMAKE_BINARY_DIR}/.intltool-merge-cache
		    ${CMAKE_SOURCE_DIR}/po
		    ${_src}
		    ${_dst}
	    DEPENDS ${_src}
	)
    else (INTLTOOL_MERGE_EXECUTABLE)
    	add_custom_command (
	    OUTPUT ${_dst}
	    COMMAND cat ${_src} |
		    sed -e 's;<_;<;g' -e 's;</_;</;g' > 
		    ${_dst}
	    DEPENDS ${_src}
	)
    endif (INTLTOOL_MERGE_EXECUTABLE)
endmacro (translate_xml)

macro (translate_desktop_file _src _dst)
    find_program (INTLTOOL_MERGE_EXECUTABLE intltool-merge)
    mark_as_advanced (FORCE INTLTOOL_MERGE_EXECUTABLE)

    if (INTLTOOL_MERGE_EXECUTABLE)
	add_custom_command (
	    OUTPUT ${_dst}
	    COMMAND ${INTLTOOL_MERGE_EXECUTABLE} -d -u -c
		    ${CMAKE_BINARY_DIR}/.intltool-merge-cache
		    ${CMAKE_SOURCE_DIR}/po
		    ${_src}
		    ${_dst}
	    DEPENDS ${_src}
	)
    else (INTLTOOL_MERGE_EXECUTABLE)
    	add_custom_command (
	    OUTPUT ${_dst}
	    COMMAND cat ${_src} |
		    sed -e 's;^_;;g' >
		    ${_dst}
	    DEPENDS ${_src}
	)
    endif (INTLTOOL_MERGE_EXECUTABLE)
endmacro (translate_desktop_file)

macro (generate_gconf_schema _src _dst)
    find_program (XSLTPROC_EXECUTABLE xsltproc)
    mark_as_advanced (FORCE XSLTPROC_EXECUTABLE)

    if (XSLTPROC_EXECUTABLE)
	add_custom_command (
	    OUTPUT ${_dst}
	    COMMAND ${XSLTPROC_EXECUTABLE}
		    --param defaultPlugins \"'$(default_plugins)'\"
	            ${compiz_SOURCE_DIR}/metadata/schemas.xslt
	            ${_src} >
	            ${_dst}
	    DEPENDS ${_src}
	)
    endif (XSLTPROC_EXECUTABLE)
endmacro (generate_gconf_schema)

macro (install_gconf_schema _file)

    find_program (GCONFTOOL_EXECUTABLE gconftool-2)
    mark_as_advanced (FORCE GCONFTOOL_EXECUTABLE)

    if (GCONFTOOL_EXECUTABLE AND NOT COMPIZ_DISABLE_SCHEMAS_INSTALL)
	install (CODE "
		if (\"\$ENV{USER}\" STREQUAL \"root\")
		    exec_program (${GCONFTOOL_EXECUTABLE}
			ARGS \"--get-default-source\"
			OUTPUT_VARIABLE ENV{GCONF_CONFIG_SOURCE})
		    exec_program (${GCONFTOOL_EXECUTABLE}
			ARGS \"--makefile-install-rule ${_file} > /dev/null\")
		else (\"\$ENV{USER}\" STREQUAL \"root\")
		    exec_program (${GCONFTOOL_EXECUTABLE}
			ARGS \"--install-schema-file=${_file} > /dev/null\")
		endif (\"\$ENV{USER}\" STREQUAL \"root\")
		")
    endif (GCONFTOOL_EXECUTABLE AND NOT COMPIZ_DISABLE_SCHEMAS_INSTALL)

    if (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
	set (SCHEMADIR "${CMAKE_INSTALL_PREFIX}/share/gconf/schemas")
    else (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
	set (SCHEMADIR "${COMPIZ_INSTALL_GCONF_SCHEMA_DIR}")
    endif (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)

    install (
	FILES ${_file}
	DESTINATION "${SCHEMADIR}"
    )
endmacro (install_gconf_schema)

macro (generate_pkg_file _src _dst)

    foreach (_val ${ARGN})
        set (_${_val}_sav ${${_val}})
        set (${_val} "")
	foreach (_word ${_${_val}_sav})
	    set (${_val} "${${_val}}${_word} ")
	endforeach (_word ${_${_val}_sav})
    endforeach (_val ${ARGN})

    configure_file (${_src} ${_dst} @ONLY)

    foreach (_val ${ARGN})
	set (${_val} ${_${_val}_sav})
        set (_${_val}_sav "")
    endforeach (_val ${ARGN})

    install (
	FILES ${_dst}
	DESTINATION ${libdir}/pkgconfig
    )
endmacro (generate_pkg_file)