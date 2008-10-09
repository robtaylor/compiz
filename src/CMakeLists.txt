include_directories (
    ${compiz_SOURCE_DIR}/include
    ${compiz_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COMPIZ_INCLUDE_DIRS}
)

add_definitions (
    -DHAVE_CONFIG_H
    -DPLUGINDIR=\\\"${compiz_plugindir}\\\"
    -DIMAGEDIR=\\\"${compiz_imagedir}\\\"
    -DMETADATADIR=\\\"${compiz_metadatadir}\\\"
)

link_directories (
    ${COMPIZ_LINK_DIRS}
)

add_executable (compiz
    region.cpp
    atoms.cpp
    timer.cpp
    main.cpp
    actions.cpp
    screen.cpp
    window.cpp
    metadata.cpp
    action.cpp
    option.cpp
    string.cpp
    match.cpp
    privates.cpp
    event.cpp
    plugin.cpp
    session.cpp
    output.cpp
    rect.cpp
    size.cpp
    point.cpp
    windowgeometry.cpp
    icon.cpp
)

target_link_libraries (
    compiz ${COMPIZ_LIBRARIES} m
)

install (
    TARGETS compiz
    DESTINATION ${exec_prefix}
)