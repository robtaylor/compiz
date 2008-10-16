include (CheckIncludeFile)

set (CMAKE_INSTALL_RPATH ${compiz_plugindir} ${libdir})

add_subdirectory (composite)
add_subdirectory (opengl)

add_library (decor MODULE decor.cpp)
target_link_libraries (decor composite opengl decoration)

add_library (move MODULE move.cpp)
target_link_libraries (move composite opengl)

add_library (resize MODULE resize.cpp)
target_link_libraries (resize composite opengl)

add_library (copytex MODULE copytex.cpp)
target_link_libraries (copytex composite opengl)

add_library (zoom MODULE zoom.cpp)
target_link_libraries (zoom composite opengl)

add_library (water MODULE water.cpp)
target_link_libraries (water composite opengl)

add_library (ini MODULE ini.cpp)

add_library (switcher MODULE switcher.cpp)
target_link_libraries (switcher composite opengl)

install (
    TARGETS decor move resize copytex zoom water ini switcher
    DESTINATION ${compiz_plugindir}
)

check_include_file ("sys/inotify.h" HAS_INOTIFY)

if (HAS_INOTIFY)
    add_library (inotify MODULE inotify.cpp)
    install (
	TARGETS inotify
	DESTINATION ${compiz_plugindir}
    )
endif (HAS_INOTIFY)

pkg_check_modules (LIBPNG libpng)
if (LIBPNG_FOUND)
    add_library (imgpng MODULE imgpng.cpp)
    target_link_libraries (imgpng ${LIBPNG_LIBRARIES})
    include_directories (
	${LIBPNG_INCLUDE_DIRS}
    )
    install (
	TARGETS imgpng
	DESTINATION ${compiz_plugindir}
    )
endif (LIBPNG_FOUND)

pkg_check_modules (LIBRSVG cairo>=1.0 cairo-xlib librsvg-2.0>=2.14.0)
if (LIBRSVG_FOUND)
    add_library (imgsvg MODULE imgsvg.cpp)
    target_link_libraries (imgsvg opengl decoration ${LIBRSVG_LIBRARIES})
    include_directories (
	${LIBRSVG_INCLUDE_DIRS}
    )
    install (
	TARGETS imgsvg
	DESTINATION ${compiz_plugindir}
    )
endif (LIBRSVG_FOUND)

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
