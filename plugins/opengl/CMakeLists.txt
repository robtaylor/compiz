find_package (OpenGL REQUIRED)

include_directories (
    ${compiz_SOURCE_DIR}/include
    ${compiz_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COMPIZ_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

link_directories (${compiz_plugindir})

add_definitions (
    -DHAVE_CONFIG_H
    -DLOCALEDIR=\\\"${datadir}/locale\\\"
    -DPLUGINDIR=\\\"${compiz_plugindir}\\\"
    -DIMAGEDIR=\\\"${compiz_imagedir}\\\"
    -DMETADATADIR=\\\"${compiz_metadatadir}\\\"
)

add_library (opengl SHARED
    opengl.cpp
    screen.cpp
    window.cpp
    paint.cpp
    texture.cpp
    fragment.cpp
    matrix.cpp
    vector.cpp
)

target_link_libraries (
    opengl composite ${OPENGL_gl_LIBRARY}
)

install (
    TARGETS opengl
    DESTINATION ${compiz_plugindir}
)