set (CMAKE_INSTALL_RPATH ${compiz_plugindir} ${libdir})

add_subdirectory (composite)
add_subdirectory (opengl)

add_library (decor MODULE decor.cpp)
target_link_libraries (decor composite opengl decoration)

add_library (move MODULE move.cpp)
target_link_libraries (move composite opengl)

add_library (resize MODULE resize.cpp)
target_link_libraries (resize composite opengl)

install (
    TARGETS decor move resize
    DESTINATION ${compiz_plugindir}
)

link_directories (${compiz_plugindir})

include_directories (
    ${compiz_SOURCE_DIR}/include
    ${compiz_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COMPIZ_INCLUDE_DIRS}
    ${_composite_plugin_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

add_definitions (
    -DHAVE_CONFIG_H
    -DLOCALEDIR=\\\"${datadir}/locale\\\"
    -DPLUGINDIR=\\\"${compiz_plugindir}\\\"
    -DIMAGEDIR=\\\"${compiz_imagedir}\\\"
    -DMETADATADIR=\\\"${compiz_metadatadir}\\\"
)
