
set (_files
    bcop.xslt
)

if (USE_GCONF)
    list (APPEND _files compiz_gconf_schemas.xslt)
endif ()

install (
    FILES ${_files}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/compiz/xslt
)