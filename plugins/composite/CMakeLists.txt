pkg_check_modules (_composite_plugin REQUIRED xdamage xcomposite)

include_directories (
    ${compiz_SOURCE_DIR}/include
    ${compiz_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COMPIZ_INCLUDE_DIRS}
    ${_composite_plugin_INCLUDE_DIRS}
)

add_definitions (
    -DHAVE_CONFIG_H
    -DLOCALEDIR=\\\"${datadir}/locale\\\"
    -DPLUGINDIR=\\\"${compiz_plugindir}\\\"
    -DIMAGEDIR=\\\"${compiz_imagedir}\\\"
    -DMETADATADIR=\\\"${compiz_metadatadir}\\\"
)

link_directories (
    ${_composite_plugin_LINK_DIRS}
)

add_library (composite SHARED
    composite.cpp
    screen.cpp
    window.cpp
)

target_link_libraries (
    composite ${_composite_plugin_LIBRARIES}
)

install (
    TARGETS composite
    DESTINATION ${compiz_plugindir}
)